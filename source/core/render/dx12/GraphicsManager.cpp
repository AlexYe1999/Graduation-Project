#include "GraphicsManager.hpp"
#include "Material.hpp"

Dx12GraphicsManager::Dx12GraphicsManager()
    : m_wnd()
    , m_frameIndex(0)
    , m_frameCount(0)
    , m_rtvDescriptorSize(0)
    , m_dsvDescriptorSize(0)
    , m_ssvDescriptorSize(0)
    , m_numConstPerFrame(0)
    , m_matConstViewHeapOffset(0)
    , m_viewport{0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}
    , m_scissors{0, 0, 0, 0}
    , m_cachedPipelineFlag(PipelineStateFlag::PIPELINE_STATE_INITIAL_FLAG)
    , m_currentPipelineFlag(PipelineStateFlag::PIPELINE_STATE_INITIAL_FLAG)
{}

void Dx12GraphicsManager::OnInit(
    uint8_t frameCount, uint16_t width, uint16_t height, HWND hWnd
){
    m_wnd            = hWnd;
    m_frameCount     = frameCount;
    m_device         = std::make_unique<Device>();
    m_commandQueue   = std::make_unique<CommandQueue>(m_device->DxDevice());
    m_frameResources = std::make_unique<FrameResource[]>(frameCount);
    m_cmdList        = m_commandQueue->GetCommandList();

    auto dxDevice = m_device->DxDevice();
    m_rtvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_ssvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    // Create SwapChain
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width       = width;
        swapChainDesc.Height      = height;
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
        {
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = m_frameCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(dxDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
        
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

            for(uint32_t n = 0; n < m_frameCount; n++){
                m_frameResources[n].rtvHandle = rtvHandle;
                rtvHandle.Offset(1, m_rtvDescriptorSize);
            }

        }

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(dxDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
        m_dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    }

    OnResize(width, height);

    // Create Deffered Rendering Rootsignature
    {
        CD3DX12_ROOT_PARAMETER rootParameter[3];

        rootParameter[0].InitAsConstantBufferView(1);
        rootParameter[1].InitAsConstantBufferView(0);
        rootParameter[2].InitAsConstantBufferView(2);
        
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
            IID_PPV_ARGS(m_rootSignatureDeferred.GetAddressOf())
        ));

    }

    // Create Shader
    {
        m_vertexShaders.push_back(std::make_unique<Dx12Shader>("BasicVertexShader", "vs_5_1"));
        m_vertexShaders.push_back(std::make_unique<Dx12Shader>("TexVertexShader", "vs_5_1"));
        m_pixelShaders.push_back(std::make_unique<Dx12Shader>("BasicPixelShader", "ps_5_1"));
        m_pixelShaders.push_back(std::make_unique<Dx12Shader>("TexPixelShader", "ps_5_1"));
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
            CreatePipelineStateObject(layoutFlag | matFlag);
        }
    }

    SetPipelineStateFlag(PipelineStateFlag::PIPELINE_STATE_DEFAULT_FLAG, 0x0, true);

}

void Dx12GraphicsManager::OnUpdate(GeoMath::Vector4f& backgroundColor){

    // Get New Frame Resource
    {
        m_frameIndex = (m_frameIndex + 1) % m_frameCount;
        m_commandQueue->WaitForFenceValue(m_frameResources[m_frameIndex].fence);

        m_cmdList = m_commandQueue->GetCommandList();
    }

    // Update New Frame Resource
    {
        auto& currFrameRes = m_frameResources[m_frameIndex];

        m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            currFrameRes.renderTarget.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
        ));

        m_cmdList->SetDescriptorHeaps(1, m_svvHeap.GetAddressOf());
        m_cmdList->SetGraphicsRootSignature(m_rootSignatureDeferred.Get());

        m_cmdList->ClearDepthStencilView(m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
        m_cmdList->ClearRenderTargetView(currFrameRes.rtvHandle, backgroundColor, 0, nullptr);

        m_cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_cmdList->RSSetViewports(1, &m_viewport);
        m_cmdList->RSSetScissorRects(1, &m_scissors);

        m_cmdList->OMSetRenderTargets(1, &currFrameRes.rtvHandle, FALSE, &m_dsvHandle);
    
    }

}

void Dx12GraphicsManager::OnResize(uint16_t width, uint16_t height){

    m_commandQueue->Flush();
    auto dxDevice = m_device->DxDevice();

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width    = width;
    m_viewport.Height   = height;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.MinDepth = 0.0f;

    m_scissors.left   = 0.0f;
    m_scissors.top    = 0.0f;
    m_scissors.right  = width;
    m_scissors.bottom = height;

	// Create RenderTarget and DepthStencil view
	{
        // Create a RTV for each frame. 
        {

            for(uint32_t n = 0; n < m_frameCount; n++){
                m_frameResources[n].renderTarget.Reset();    
            }

            DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
            ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
            ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(
                m_frameCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags
            ));

            for(uint32_t n = 0; n < m_frameCount; n++){
                auto& frameResource = m_frameResources[n];

                frameResource.renderTarget.Reset();
                ThrowIfFailed(m_dxgiSwapChain->GetBuffer(n, IID_PPV_ARGS(frameResource.renderTarget.GetAddressOf())));
                dxDevice->CreateRenderTargetView(frameResource.renderTarget.Get(), nullptr, frameResource.rtvHandle);
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
            dsResDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dsResDesc.SampleDesc.Count = 1;
            dsResDesc.SampleDesc.Quality = 0;
            dsResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            dsResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_CLEAR_VALUE clearValueDs = {};
            clearValueDs.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            clearValueDs.DepthStencil.Depth = 1.0f;
            clearValueDs.DepthStencil.Stencil = 0;

            m_depthStencil.Reset();
            dxDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                &dsResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearValueDs, IID_PPV_ARGS(m_depthStencil.GetAddressOf())
            );

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            dsvDesc.Texture2D.MipSlice = 0;

            dxDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvHandle);
        }
	}

    m_frameIndex = (m_dxgiSwapChain->GetCurrentBackBufferIndex() + m_frameCount - 1) % m_frameCount;

}

void Dx12GraphicsManager::CreateRenderResource(
    size_t numObject, 
    size_t numMat,  const UploadBuffer& matResource,
    size_t numTex
){

    auto dxDevice = m_device->DxDevice();

    m_numConstPerFrame = numObject + 1;

    // Create Constant buffer descriptorHeap
    D3D12_DESCRIPTOR_HEAP_DESC svvHeapDesc = {};
    svvHeapDesc.NumDescriptors = m_frameCount * m_numConstPerFrame + numMat + numTex;
    svvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    svvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(dxDevice->CreateDescriptorHeap(&svvHeapDesc, IID_PPV_ARGS(&m_svvHeap)));

    // Create Perframe Resource
    {

        constexpr size_t mainConstByteSize = Utility::CalcAlignment<256>(sizeof(MainConstBuffer));
        constexpr size_t objectConstByteSize = Utility::CalcAlignment<256>(sizeof(ObjectConstBuffer));
    
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        for(size_t index = 0; index < m_frameCount; index++){

            auto& frameResource = m_frameResources[index];
            CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_svvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create Main Constant buffer and view
            frameResource.mainConst = std::make_unique<UploadBuffer>(dxDevice, mainConstByteSize);

            cbvHandle.Offset(index * m_numConstPerFrame, m_ssvDescriptorSize);
            cbvDesc.BufferLocation = frameResource.mainConst->GetAddress();
            cbvDesc.SizeInBytes = mainConstByteSize;

            dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);

            frameResource.objectConst = std::make_unique<UploadBuffer>(dxDevice, objectConstByteSize * numObject);

            // Create Object Constant buffer and view
            size_t virtualAddress = frameResource.objectConst->GetAddress();
            for(size_t offset = 0; offset < numObject; offset++){

                cbvHandle.Offset(1, m_ssvDescriptorSize);
                cbvDesc.BufferLocation = virtualAddress;
                cbvDesc.SizeInBytes = objectConstByteSize;

                dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
                virtualAddress += objectConstByteSize;
            }

        }

    }

    constexpr size_t matConstByteSize = Utility::CalcAlignment<256>(sizeof(MaterialConstant));
    m_matConstViewHeapOffset = m_numConstPerFrame * m_frameCount;
    m_matConstBuffer = std::make_unique<DefaultBuffer>(dxDevice, m_cmdList, matResource);

    D3D12_CONSTANT_BUFFER_VIEW_DESC ssvDesc = {};
    CD3DX12_CPU_DESCRIPTOR_HANDLE ssvHandle(
        m_svvHeap->GetCPUDescriptorHandleForHeapStart(), 
        m_matConstViewHeapOffset, m_ssvDescriptorSize
    );
    size_t virtualAddress = m_matConstBuffer->GetAddress();

    for(uint32_t index = 0; index < numMat; index++){

        ssvDesc.BufferLocation = virtualAddress;
        ssvDesc.SizeInBytes = matConstByteSize;

        dxDevice->CreateConstantBufferView(&ssvDesc, ssvHandle);
        virtualAddress += matConstByteSize;
        ssvHandle.Offset(1, m_ssvDescriptorSize);
    }

}

void Dx12GraphicsManager::CreatePipelineStateObject(uint64_t flag){

    auto& psoIt = m_pipelineStateObjects.find(flag);

    if(psoIt == m_pipelineStateObjects.end()){
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        switch(flag & 0x7){
            case PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_0 :
            {
                psoDesc.InputLayout = {
                    vertex0.inputLayout.data(), 
                    static_cast<unsigned int>(vertex0.inputLayout.size())
                };
                psoDesc.VS = m_vertexShaders[0]->GetByteCode();
                psoDesc.PS = m_pixelShaders[0]->GetByteCode();
                break;
            }
            case PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_1 :
            {
                psoDesc.InputLayout = {
                    vertex1.inputLayout.data(),
                    static_cast<unsigned int>(vertex1.inputLayout.size())
                };
                psoDesc.VS = m_vertexShaders[1]->GetByteCode();
                psoDesc.PS = m_pixelShaders[1]->GetByteCode();
                break;
            }
            default:
                break;
        }

        switch(flag & 0x18){
            case PipelineStateFlag::PIPELINE_STATE_CULL_MODE_FRONT :
            {
                psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
                break;
            }
            case PipelineStateFlag::PIPELINE_STATE_CULL_MODE_BACK :
            {
                psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
                break;
            }
            default:
            {
                psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
                break;
            }

        }

        psoDesc.pRootSignature = m_rootSignatureDeferred.Get();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.FrontCounterClockwise = (flag & 0x20) > 0;
        psoDesc.RasterizerState.MultisampleEnable = false;
        psoDesc.RasterizerState.DepthClipEnable = true;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        ThrowIfFailed(m_device->DxDevice()->CreateGraphicsPipelineState(
            &psoDesc, IID_PPV_ARGS(m_pipelineStateObjects[flag].GetAddressOf()))
        );
    }
}

void Dx12GraphicsManager::SetPipelineStateFlag(uint64_t flag, uint64_t mask, bool finalFlag){

    m_currentPipelineFlag = (m_currentPipelineFlag & ~mask) | flag;

    if(finalFlag && m_currentPipelineFlag != m_cachedPipelineFlag){
        m_cmdList->SetPipelineState(m_pipelineStateObjects[m_currentPipelineFlag].Get());
        m_cachedPipelineFlag  = m_currentPipelineFlag;
    }

}