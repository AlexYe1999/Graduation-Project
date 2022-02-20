#include "AppFramework.hpp"
#include "d3dx12.h"
#include "Pipeline.hpp"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <chrono>

Pipeline::Pipeline(std::string& name, uint16_t width, uint16_t height)
    : Application(name, width, height)
    , m_frameIndex(0)
    , m_rtvDescriptorSize(0)
{}

Pipeline::~Pipeline(){}

void Pipeline::OnInit(){
    InitD3D();
    InitGUI();
}

void Pipeline::OnTick(){
    static const std::chrono::duration<double, std::ratio<1, 60>> fps60(1);
    static std::chrono::time_point<std::chrono::steady_clock> startTime;
    static std::chrono::time_point<std::chrono::steady_clock> endTime;

    endTime = std::chrono::steady_clock::now();
    if(endTime - startTime > fps60){
        startTime = std::chrono::steady_clock::now();
        OnUpdate();
        OnRender();    
    }
}

void Pipeline::OnUpdate(){

}

void Pipeline::OnRender(){
    m_d3d12CommandList = m_commandQueue->GetCommandList();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = m_renderTargets[m_frameIndex].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    m_d3d12CommandList->ResourceBarrier(1, &barrier);

    const float clear_color_with_alpha[4] = {0.45f, 0.55f, 0.60f, 1.00f};
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

    m_d3d12CommandList->ClearRenderTargetView(rtvHandle, clear_color_with_alpha, 0, nullptr);
    m_d3d12CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    RenderScene();
    RenderGUI();

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    m_d3d12CommandList->ResourceBarrier(1, &barrier);

    m_commandQueue->ExecuteCommandList(m_d3d12CommandList);
    m_dxgiSwapChain->Present(1, 0); 
    m_commandQueue->Flush();

    m_frameIndex = (m_frameIndex+1) % m_frameCount;
}

void Pipeline::OnDestroy(){
    m_commandQueue->Flush();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Pipeline::OnResize(const uint16_t width, const uint16_t height){
    m_commandQueue->Flush();


}

void Pipeline::OnKeyDown(const uint16_t key, const int16_t x, const int16_t y){

}

void Pipeline::OnKeyUp(const uint16_t key, const int16_t x, const int16_t y){

}

// Forward declare message handler from imgui_impl_win32.cpp
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
            OnKeyDown(static_cast<uint16_t>(wParam));
            return 0;
        }
        case WM_KEYUP:
        {
            OnKeyUp(static_cast<uint16_t>(wParam));
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

void Pipeline::InitD3D(){

    {
        m_device       = std::make_unique<Device>();
        m_commandQueue = std::make_unique<CommandQueue>(m_device->DxDevice());
    }
    
    // Create SwapChain
    
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width       = m_wndWidth;
        swapChainDesc.Height      = m_wndHeight;
        swapChainDesc.BufferCount = m_frameCount;
        swapChainDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(
            m_device->DxgiFactory()->CreateSwapChainForHwnd(
                m_commandQueue->GetD3D12CommandQueue().Get(), AppFramework::GetWnd(),
                &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()
            )
        );

        ThrowIfFailed(swapChain.As(&m_dxgiSwapChain));

        m_frameIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
    }

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = m_frameCount;
		rtvHeapDesc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->DxDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->DxDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (uint32_t n = 0; n < m_frameCount; n++) {
			ThrowIfFailed(m_dxgiSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->DxDevice()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

}

void Pipeline::InitGUI(){

    D3D12_DESCRIPTOR_HEAP_DESC guiDescHeapDesc = {};
    guiDescHeapDesc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    guiDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    guiDescHeapDesc.NumDescriptors = 1;
    m_device->DxDevice()->CreateDescriptorHeap(&guiDescHeapDesc, IID_PPV_ARGS(m_guiSrvDescHeap.GetAddressOf()));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(AppFramework::GetWnd());
    ImGui_ImplDX12_Init(m_device->DxDevice().Get(), m_frameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM, m_guiSrvDescHeap.Get(),
        m_guiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        m_guiSrvDescHeap->GetGPUDescriptorHandleForHeapStart()
    );

}

void Pipeline::LoadContent(){

}

void Pipeline::RenderScene(){

}

void Pipeline::RenderGUI(){

    m_d3d12CommandList->SetDescriptorHeaps(1, m_guiSrvDescHeap.GetAddressOf());

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = true;
    static bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    if(show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

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
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if(ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_d3d12CommandList.Get());
}