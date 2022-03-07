#include "RenderResource.hpp"

RenderResource::RenderResource(uint8_t frameCount, uint16_t width, uint16_t height, HWND hWnd)
    : m_wnd(hWnd)
    , m_frameIndex(0)
    , m_frameCount(frameCount)
    , m_width(width)
    , m_height(height)
    , m_rtvDescriptorSize(0)
    , m_dsvDescriptorSize(0)
    , m_ssvDescriptorSize(0)
    , m_numConstPerFrame(0)
    , m_device(new Device)
    , m_commandQueue(new CommandQueue(m_device->DxDevice()))
    , m_frameResources(new FrameResource[frameCount])
    , m_cmdList(m_commandQueue->GetCommandList())
{

    auto dxDevice = m_device->DxDevice();
    m_rtvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_ssvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    // Create SwapChain
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

    // Create render target and DepthStencil descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = m_frameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(dxDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(dxDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
    }

	// Create RenderTarget and DepthStencil view
	{
        // Create a RTV for each frame. 
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
            for(uint32_t n = 0; n < m_frameCount; n++){
                auto& frameResource = m_frameResources[n];

                frameResource.rtvHandle = rtvHandle;
                ThrowIfFailed(m_dxgiSwapChain->GetBuffer(n, IID_PPV_ARGS(frameResource.renderTarget.GetAddressOf())));
                dxDevice->CreateRenderTargetView(frameResource.renderTarget.Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, m_rtvDescriptorSize);
            }

        }

        // Create DepthStencil
        {
            D3D12_RESOURCE_DESC dsResDesc = {};
            dsResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            dsResDesc.Alignment = 0;
            dsResDesc.Width = width;
            dsResDesc.Height = height;
            dsResDesc.DepthOrArraySize = 1;
            dsResDesc.MipLevels = 1;
            dsResDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsResDesc.SampleDesc.Count = 1;
            dsResDesc.SampleDesc.Quality = 0;
            dsResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            dsResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_CLEAR_VALUE clearValueDs = {};
            clearValueDs.Format = DXGI_FORMAT_D32_FLOAT;
            clearValueDs.DepthStencil.Depth = 1.0f;
            clearValueDs.DepthStencil.Stencil = 0;

            dxDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                &dsResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearValueDs, IID_PPV_ARGS(m_depthStencil.GetAddressOf())
            );

            dsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            dsvDesc.Texture2D.MipSlice = 0;
            dxDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, dsvHandle);

        }

	}

    m_frameIndex = (m_dxgiSwapChain->GetCurrentBackBufferIndex() + m_frameCount - 1) % m_frameCount;

    // Create Deffered Rendering Rootsignature
    {
        CD3DX12_ROOT_PARAMETER rootParameter[2];

        rootParameter[0].InitAsConstantBufferView(1);
        rootParameter[1].InitAsConstantBufferView(0);
        
        const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
            0,                                // shaderRegister
            D3D12_FILTER_ANISOTROPIC,         // filter
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
            0.0f,                             // mipLODBias
            8                                 // maxAnisotropy
        );                               

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
            2, rootParameter, 1, &anisotropicWrap,
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
            IID_PPV_ARGS(rootSignatureDeferred.GetAddressOf())
        ));

    }

    // Create Shader
    {
        vertexShaders[0] = std::make_unique<Dx12Shader>("BasicVertexShader", "vs_5_1");
        vertexShaders[1] = std::make_unique<Dx12Shader>("TexVertexShader", "vs_5_1");
        pixelShaders[0]  = std::make_unique<Dx12Shader>("BasicPixelShader", "ps_5_1");
        pixelShaders[1]  = std::make_unique<Dx12Shader>("TexPixelShader", "ps_5_1");
    }

}

void RenderResource::CreateRenderResource(size_t numObject, size_t numMat, size_t numTex){

    auto dxDevice = m_device->DxDevice();

    m_numConstPerFrame = numObject + 1;

    // Create Constant buffer descriptorHeap
    D3D12_DESCRIPTOR_HEAP_DESC ssvHeapDesc = {};
    ssvHeapDesc.NumDescriptors = m_frameCount * m_numConstPerFrame + numMat + numTex;
    ssvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ssvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(dxDevice->CreateDescriptorHeap(&ssvHeapDesc, IID_PPV_ARGS(&ssvHeap)));

    constexpr size_t mainConstByteSize   = Utils::CalcAlignment<256>(sizeof(MainConstBuffer));
    constexpr size_t objectConstByteSize = Utils::CalcAlignment<256>(sizeof(ObjectConstBuffer));
    
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    for(size_t index = 0; index < m_frameCount; index++){

        auto& frameResource = m_frameResources[index];
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(ssvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create Main Constant buffer and view
        frameResource.mainConst = std::make_unique<UploadBuffer>(dxDevice, mainConstByteSize);

        cbvHandle.Offset(index * m_numConstPerFrame, m_ssvDescriptorSize);
        cbvDesc.BufferLocation = frameResource.mainConst->GetAddress();
        cbvDesc.SizeInBytes = mainConstByteSize;

        dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);

        frameResource.objectConst = std::make_unique<UploadBuffer>(dxDevice, objectConstByteSize * numObject);

        // Create Object Constant buffer and view
        for(size_t offset = 0; offset < numObject; offset++){

            cbvHandle.Offset(1, m_ssvDescriptorSize);
            cbvDesc.BufferLocation = frameResource.objectConst->GetAddress() + offset * objectConstByteSize;
            cbvDesc.SizeInBytes    = objectConstByteSize;
        
            dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
        }

    }

}
