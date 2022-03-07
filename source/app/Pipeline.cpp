#include "AppFramework.hpp"
#include "Model.hpp"
#include "Pipeline.hpp"
#include "d3dx12.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

#include <chrono>

Pipeline::Pipeline(std::string& name, uint16_t width, uint16_t height)
    : Application(name, width, height)
    , m_isLeftMouseDown(false)
    , m_lastMousePosX(0)
    , m_lastMousePosY(0)
    , m_camera(nullptr)
    , m_viewport{0.0f, 0.0f, static_cast<float>(m_wndWidth), static_cast<float>(m_wndHeight), 0.0f, 1.0f}
    , m_scissors{0, 0, m_wndWidth, m_wndHeight}
    , m_backgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
{}

Pipeline::~Pipeline(){}

void Pipeline::OnInit(){

    InitD3D();
    InitGUI();
    LoadContent();

}

void Pipeline::InitD3D(){
    m_renderResource = std::make_unique<RenderResource>(FrameCount, m_wndWidth, m_wndHeight, AppFramework::GetWnd());
}

void Pipeline::InitGUI(){

    auto dxDevice = m_renderResource->GetDevice();
    D3D12_DESCRIPTOR_HEAP_DESC guiDescHeapDesc = {};
    guiDescHeapDesc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    guiDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    guiDescHeapDesc.NumDescriptors = 1;
    dxDevice->CreateDescriptorHeap(&guiDescHeapDesc, IID_PPV_ARGS(m_guiSrvDescHeap.GetAddressOf()));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(AppFramework::GetWnd());
    ImGui_ImplDX12_Init(dxDevice.Get(), FrameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM, m_guiSrvDescHeap.Get(),
        m_guiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        m_guiSrvDescHeap->GetGPUDescriptorHandleForHeapStart()
    );

}

void Pipeline::LoadContent(){

    auto cmdList = m_renderResource->GetCommandList();

    SceneNode::SetFrameCount(FrameCount);
    auto model = Dx12Model("assets\\gltf\\scene.gltf", m_renderResource.get());

    m_scene = std::move(model.root);
    m_camera = new Dx12Camera(0, m_scene.get(), m_renderResource.get());
    m_camera->SetMatrix(nullptr, &GeoMath::Matrix4f::Rotation(0.0f, 0.7f, 0.0f), &GeoMath::Matrix4f::Translation(0.0, 0.0, -50.0f));

    m_scene->AddChild(std::unique_ptr<SceneNode>(m_camera));

    m_renderResource->ExecuteCommandList(cmdList);
    m_renderResource->Flush();
}

void Pipeline::OnTick(){
    static const std::chrono::duration<double, std::ratio<1, 60>> deltaTime(1);
    static std::chrono::time_point<std::chrono::steady_clock> startTime;
    static std::chrono::time_point<std::chrono::steady_clock> endTime;

    endTime = std::chrono::steady_clock::now();
    if(endTime - startTime > deltaTime){
        startTime = std::chrono::steady_clock::now();
        OnUpdate();
        OnRender();
    }
}

void Pipeline::OnUpdate(){
    m_renderResource->OnUpdate();
    m_scene->OnUpdate();
}

void Pipeline::OnRender(){

    auto& currFrameRes = m_renderResource->GetFrameResource();
    auto  cmdList      = m_renderResource->GetCommandList();
    
    cmdList->SetDescriptorHeaps(1, m_renderResource->ssvHeap.GetAddressOf());
    cmdList->SetGraphicsRootSignature(m_renderResource->rootSignatureDeferred.Get());

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        currFrameRes.renderTarget.Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    cmdList->ClearDepthStencilView(m_renderResource->dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    cmdList->ClearRenderTargetView(currFrameRes.rtvHandle, m_backgroundColor, 0, nullptr);

    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissors);

    cmdList->OMSetRenderTargets(1, &currFrameRes.rtvHandle, FALSE, &m_renderResource->dsvHandle);

    RenderScene();
    //RenderGUI();

   cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        currFrameRes.renderTarget.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
    ));

   m_renderResource->OnRender(cmdList);

}

void Pipeline::RenderScene(){
    auto& currFrameRes = m_renderResource->GetFrameResource();
    auto  cmdList      = m_renderResource->GetCommandList();
    cmdList->SetGraphicsRootConstantBufferView(1, currFrameRes.mainConst->GetAddress()); 
    
    m_scene->OnRender();
}

void Pipeline::RenderGUI(){

    auto cmdList = m_renderResource->GetCommandList();

    cmdList->SetDescriptorHeaps(1, m_guiSrvDescHeap.GetAddressOf());

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window    = true;
    static bool show_another_window = false;
    if(show_demo_window){
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", m_backgroundColor); // Edit 3 floats representing a color

        if(ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if(show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if(ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());
}

void Pipeline::OnDestroy(){
    m_renderResource->Flush();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Pipeline::OnResize(const uint16_t width, const uint16_t height){
    m_wndWidth  = width;
    m_wndHeight = height;

    m_renderResource->OnResize();

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width    = m_wndWidth;
    m_viewport.Height   = m_wndHeight;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.MinDepth = 0.0f;

    m_scissors.left   = 0.0f;
    m_scissors.top    = 0.0f;
    m_scissors.right  = m_wndWidth;
    m_scissors.bottom = m_wndHeight;

    float aspRatio = GetAspectRatio();
    m_camera->SetLens(nullptr, nullptr, &aspRatio, nullptr);
}

void Pipeline::OnKeyDown(const uint16_t key, const int16_t x, const int16_t y){

    switch(key){
        case WM_LBUTTONDOWN:
            m_isLeftMouseDown = true;
            break;
        case WM_MOUSEMOVE:
        {
            auto& io = ImGui::GetIO();
            if(m_isLeftMouseDown && !io.WantCaptureKeyboard && !io.WantCaptureMouse){
                m_camera->Input(key, x - m_lastMousePosX, y - m_lastMousePosY);
            }
            m_lastMousePosX = x;
            m_lastMousePosY = y;
            break;
        }
        case WM_KEYDOWN:
            m_camera->Input(x);
            break;
        default:
            break;
    }

}

void Pipeline::OnKeyUp(const uint16_t key, const int16_t x, const int16_t y){
    switch(key){
        case WM_LBUTTONUP:
            m_isLeftMouseDown = false;
            break;

        default:
            break;
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT Pipeline::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){

    if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)){
        return true;
    }

    switch (msg){
        case WM_SIZE:
        {
            OnResize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            float t = GET_WHEEL_DELTA_WPARAM(wParam);
            OnKeyDown(static_cast<uint16_t>(WM_MOUSEWHEEL), GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            OnKeyDown(static_cast<uint16_t>(WM_MOUSEMOVE), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_LBUTTONDOWN:
        {
            SetCapture(hWnd);
            OnKeyDown(static_cast<uint16_t>(WM_LBUTTONDOWN), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_LBUTTONUP:
        {
            OnKeyUp(static_cast<uint16_t>(WM_LBUTTONUP), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            ReleaseCapture();
            return 0;
        }
        case WM_KEYDOWN:
        {
            OnKeyDown(static_cast<uint16_t>(WM_KEYDOWN), static_cast<uint16_t>(wParam));
            return 0;
        }
        case WM_KEYUP:
        {
            OnKeyUp(static_cast<uint16_t>(WM_KEYUP), static_cast<uint16_t>(wParam));
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        default:
        {
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }
}

