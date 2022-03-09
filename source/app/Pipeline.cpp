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
    , m_cameraMode(0)
    , m_camera(nullptr)
    , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
    , m_backgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
{}

Pipeline::~Pipeline(){}

void Pipeline::OnInit(){
    InitD3D();
    InitGUI();
    LoadContent();
}

void Pipeline::InitD3D(){
    m_graphicsMgr->OnInit(FrameCount, m_wndWidth, m_wndHeight, AppFramework::GetWnd());
}

void Pipeline::InitGUI(){

    auto dxDevice = m_graphicsMgr->GetDevice();
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

    auto cmdList = m_graphicsMgr->GetCommandList();
    auto model = Dx12Model("assets\\gltf\\scene.gltf");

    m_scene = std::move(model.root);
    m_scene->SetMatrix(&GeoMath::Matrix4f::Scale(1.0f, -1.0f, 1.0f), nullptr, nullptr);

    float aspRatio = GetAspectRatio();
    m_camera = new Dx12Camera(0, m_scene.get());
    m_camera->SetLens(nullptr, nullptr, &aspRatio, nullptr);
    m_camera->SetMatrix(nullptr, nullptr, &GeoMath::Matrix4f::Translation(0.0, 5.0, -10.0f));

    m_scene->AddChild(std::unique_ptr<SceneNode>(m_camera));

    m_graphicsMgr->ExecuteCommandList(cmdList);
    m_graphicsMgr->Flush();
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
    m_graphicsMgr->OnUpdate(m_backgroundColor);
    m_scene->OnUpdate();
}

void Pipeline::OnRender(){

    RenderScene();
    RenderGUI();

    m_graphicsMgr->OnRender();

}

void Pipeline::RenderScene(){
    auto& currFrameRes = m_graphicsMgr->GetFrameResource();
    auto  cmdList      = m_graphicsMgr->GetCommandList();
    
    cmdList->SetGraphicsRootConstantBufferView(1, currFrameRes.mainConst->GetAddress()); 
    
    m_scene->OnRender();
}

void Pipeline::RenderGUI(){

    auto cmdList = m_graphicsMgr->GetCommandList();

    cmdList->SetDescriptorHeaps(1, m_guiSrvDescHeap.GetAddressOf());

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window    = true;
    static bool show_another_window = false;
    if(show_demo_window){
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    {
        ImGui::Begin("Control Panel");
        ImGui::ColorEdit3("Background Color", m_backgroundColor);

        ImGui::SliderInt("Camera Mode", &m_cameraMode, 1, 3);  

        ImGui::SameLine();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());
}

void Pipeline::OnDestroy(){
    m_graphicsMgr->Flush();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Pipeline::OnResize(const uint16_t width, const uint16_t height){
    m_wndWidth  = max(1, width);
    m_wndHeight = max(1, height);

    float aspRatio = GetAspectRatio();
    m_camera->SetLens(nullptr, nullptr, &aspRatio, nullptr);
    m_graphicsMgr->OnResize(m_wndWidth, m_wndHeight);
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

