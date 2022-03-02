#include "RenderResource.hpp"

RenderResource::RenderResource(uint8_t frameCount, uint16_t width, uint16_t height, HWND hWnd)
    : m_wnd(hWnd)
    , m_width(width)
    , m_height(height)
    , m_frameIndex(0)
    , m_frameCount(frameCount)
    , m_rtvDescriptorSize(0)
    , m_device(new Device)
    , m_commandQueue(new CommandQueue(m_device->DxDevice()))
    , m_frameResources(new FrameResource[frameCount])
    , m_cmdList(m_commandQueue->GetCommandList())
{

    auto dxDevice = m_device->DxDevice();
    m_rtvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create SwapChain and renderResource
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width       = m_width;
        swapChainDesc.Height      = m_height;
        swapChainDesc.BufferCount = frameCount;
        swapChainDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(
            m_device->DxgiFactory()->CreateSwapChainForHwnd(
                m_commandQueue->GetD3D12CommandQueue().Get(), hWnd,
                &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()
            )
        );

        ThrowIfFailed(swapChain.As(&m_dxgiSwapChain));

    }


    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = m_frameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(dxDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
    }

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (uint32_t n = 0; n < m_frameCount; n++) {
            auto& frameResource = m_frameResources[n];

            frameResource.rtvHandle = rtvHandle;
			ThrowIfFailed(m_dxgiSwapChain->GetBuffer(n, IID_PPV_ARGS(frameResource.renderTarget.GetAddressOf())));
            dxDevice->CreateRenderTargetView(frameResource.renderTarget.Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

    m_frameIndex = (m_dxgiSwapChain->GetCurrentBackBufferIndex() + m_frameCount - 1) % m_frameCount;

}
