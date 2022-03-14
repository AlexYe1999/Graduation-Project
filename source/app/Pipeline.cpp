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
    , m_rtvDescriptorSize(0)
    , m_dsvDescriptorSize(0)
    , m_svvDescriptorSize(0)
    , m_texConstViewOffset(0)
    , m_gbufferViewOffset(0)
    , m_backgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
    , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
    , m_viewport{}
    , m_scissors{}
{}

Pipeline::~Pipeline(){}

void Pipeline::OnInit(){
    InitD3D();
    InitGUI();
}

void Pipeline::InitD3D(){

    m_graphicsMgr->OnInit(AppFramework::GetWnd(), 3, m_wndWidth, m_wndHeight);

    auto& dxDevice = m_graphicsMgr->GetDevice();
    auto  cmdList  = m_graphicsMgr->GetCommandList();
    
    m_rtvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_svvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create Deffered Rendering Rootsignature
    {
        CD3DX12_ROOT_PARAMETER rootParameter[4] = {};
        D3D12_DESCRIPTOR_RANGE range{
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            5, 0, 0, 0
        };

        rootParameter[0].InitAsConstantBufferView(1);
        rootParameter[1].InitAsConstantBufferView(0);
        rootParameter[2].InitAsConstantBufferView(2);
        rootParameter[3].InitAsDescriptorTable(1, &range);

        const D3D12_STATIC_SAMPLER_DESC samplers[2] = {
            CD3DX12_STATIC_SAMPLER_DESC(
                0,                                // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP   // addressW
            ),
            CD3DX12_STATIC_SAMPLER_DESC(
                1,                                // shaderRegister
                D3D12_FILTER_ANISOTROPIC,         // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
                0.0f,                             // mipLODBias
                8                                 // maxAnisotropy
            )
        };

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
            4, rootParameter, 2, samplers,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        );

        ComPtr<ID3DBlob> serializedRootSignature = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        ThrowIfFailed(D3D12SerializeRootSignature(
            &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
            serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf()
        ));

        ThrowIfFailed(dxDevice->CreateRootSignature(
            0,
            serializedRootSignature->GetBufferPointer(),
            serializedRootSignature->GetBufferSize(),
            IID_PPV_ARGS(m_deferredRootSignature.GetAddressOf())
        ));

    }

    uint64_t layoutFlags[2] = {
        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_0,
        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_1
    };

    uint64_t matFlags[2] = {
        PipelineStateFlag::PIPELINE_STATE_CULL_MODE_NONE | PipelineStateFlag::PIPELINE_STATE_FRONT_COUNTER_CLOCKWISE,
        PipelineStateFlag::PIPELINE_STATE_CULL_MODE_BACK | PipelineStateFlag::PIPELINE_STATE_FRONT_COUNTER_CLOCKWISE
    };

    for(auto layoutFlag : layoutFlags){
        for(auto matFlag : matFlags){
            m_graphicsMgr->CreatePipelineStateObject(
                PipelineStateFlag::PIPELINE_STATE_RENDER_GBUFFER | layoutFlag | matFlag,
                m_deferredRootSignature
            );
        }
    }

    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_2  |
        PipelineStateFlag::PIPELINE_STATE_CULL_MODE_BACK |
        PipelineStateFlag::PIPELINE_STATE_FRONT_CLOCKWISE, 
        m_deferredRootSignature
    );

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(dxDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

    // Create Scene Resource
    auto model = Dx12Model("assets\\gltf\\scene.gltf");

    m_scene = std::move(model.root);
    m_scene->SetMatrix(&GeoMath::Matrix4f::Scale(1.0f, -1.0f, 1.0f), nullptr, nullptr);

    m_cbvHeap = std::move(model.cbvHeap);
    m_matConstBuffer = std::move(model.matConstBuffer);

    float aspRatio = GetAspectRatio();
    m_camera = new Dx12Camera(0, m_scene.get());
    m_camera->SetLens(nullptr, nullptr, &aspRatio, nullptr);
    m_camera->SetMatrix(nullptr, nullptr, &GeoMath::Matrix4f::Translation(0.0, 5.0, -10.0f));

    m_scene->AddChild(std::unique_ptr<SceneNode>(m_camera));

    m_graphicsMgr->ExecuteCommandList(cmdList);
    m_graphicsMgr->Flush();
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
    ImGui_ImplDX12_Init(dxDevice.Get(), 3,
        DXGI_FORMAT_R8G8B8A8_UNORM, m_guiSrvDescHeap.Get(),
        m_guiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        m_guiSrvDescHeap->GetGPUDescriptorHandleForHeapStart()
    );

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

void Pipeline::LoadContent(){

    auto cmdList = m_graphicsMgr->GetCommandList();
    auto model = Dx12Model("assets\\gltf\\scene.gltf");

    m_scene = std::move(model.root);
    m_scene->SetMatrix(&GeoMath::Matrix4f::Scale(1.0f, -1.0f, 1.0f), nullptr, nullptr);

    m_cbvHeap = std::move(model.cbvHeap);
    m_matConstBuffer = std::move(model.matConstBuffer);

    float aspRatio = GetAspectRatio();
    m_camera = new Dx12Camera(0, m_scene.get());
    m_camera->SetLens(nullptr, nullptr, &aspRatio, nullptr);
    m_camera->SetMatrix(nullptr, nullptr, &GeoMath::Matrix4f::Translation(0.0, 5.0, -10.0f));

    m_scene->AddChild(std::unique_ptr<SceneNode>(m_camera));

}

void Pipeline::OnUpdate(){
    m_graphicsMgr->OnUpdate();
    m_scene->OnUpdate();
}

void Pipeline::OnRender(){

    auto& currFrameRes = m_graphicsMgr->GetFrameResource();
    auto  cmdList      = m_graphicsMgr->GetCommandList();

    m_graphicsMgr->SetPipelineStateFlag(
        PipelineStateFlag::PIPELINE_STATE_RENDER_GBUFFER, 0x7000, false
    );

    D3D12_VIEWPORT viewPorts[5] = {
        m_viewport, m_viewport, m_viewport,
        m_viewport, m_viewport
    };

    D3D12_RECT scissors[5] = {
        m_scissors, m_scissors, m_scissors,
        m_scissors, m_scissors
    };

    cmdList->SetGraphicsRootSignature(m_deferredRootSignature.Get());
    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->RSSetViewports(5, viewPorts);
    cmdList->RSSetScissorRects(5, scissors);

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    auto [rtvHangles, numGBuffer] = currFrameRes.gbuffer->AsRenderTarget(cmdList);
    cmdList->OMSetRenderTargets(numGBuffer, rtvHangles, FALSE, &dsvHandle);

    RenderScene();

    // Render to back buffer
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        currFrameRes.renderTarget.Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    m_graphicsMgr->SetPipelineStateFlag(
        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_2  |
        PipelineStateFlag::PIPELINE_STATE_CULL_MODE_BACK |
        PipelineStateFlag::PIPELINE_STATE_FRONT_CLOCKWISE,
        0xFFFFFFFF, true
    );

    cmdList->IASetIndexBuffer(nullptr);
    cmdList->IASetVertexBuffers(0, 0, nullptr);

    cmdList->SetDescriptorHeaps(1, currFrameRes.gbuffer->GetSRVHeap().GetAddressOf());
    cmdList->SetGraphicsRootDescriptorTable(
        3, currFrameRes.gbuffer->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart()
    );

    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissors);

    cmdList->ClearRenderTargetView(currFrameRes.rtvHandle, m_backgroundColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &currFrameRes.rtvHandle, FALSE, nullptr);

    currFrameRes.gbuffer->AsPixelShaderResource(cmdList);
    cmdList->DrawInstanced(3, 1, 0, 0);

    RenderGUI();

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        currFrameRes.renderTarget.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
    ));

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
        ImGui::Separator();

        ImGui::SliderInt("Camera Mode", &m_cameraMode, 1, 3);
        ImGui::Separator();

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

    auto& dxDevice = m_graphicsMgr->GetDevice();

    m_wndWidth  = max(1, width);
    m_wndHeight = max(1, height);

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
    
    // Create DepthStencil 
    {
        D3D12_RESOURCE_DESC dsResDesc = {};
        dsResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dsResDesc.Alignment = 0;
        dsResDesc.Width  = width;
        dsResDesc.Height = height;
        dsResDesc.DepthOrArraySize = 1;
        dsResDesc.MipLevels = 1;
        dsResDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsResDesc.SampleDesc.Count   = 1;
        dsResDesc.SampleDesc.Quality = 0;
        dsResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        dsResDesc.Flags  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValueDs = {};
        clearValueDs.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValueDs.DepthStencil.Depth   = 1.0f;
        clearValueDs.DepthStencil.Stencil = 0;

        m_depthStencil.Reset();
        dxDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
            &dsResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValueDs, IID_PPV_ARGS(m_depthStencil.GetAddressOf())
        );

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags         = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        dxDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, dsvHandle);
    }

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

