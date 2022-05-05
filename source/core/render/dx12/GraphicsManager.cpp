#include "GraphicsManager.hpp"
#include "Material.hpp"

#include <random>
#include <chrono>

Dx12GraphicsManager::Dx12GraphicsManager()
    : m_frameIndex(0)
    , m_frameCount(0)
    , m_cachedPipelineFlag(PipelineStateFlag::PIPELINE_STATE_INITIAL_FLAG)
    , m_currentPipelineFlag(PipelineStateFlag::PIPELINE_STATE_INITIAL_FLAG)
{}

void Dx12GraphicsManager::OnInit(
    HWND hWnd, uint8_t frameCount, uint16_t width, uint16_t height
){
    m_frameCount      = frameCount;
    m_device          = std::make_unique<Device>();
    m_commandQueue    = std::make_unique<CommandQueue>(m_device->DxDevice());
    m_frameResources  = std::make_unique<FrameResource[]>(frameCount);
    m_cmdList         = m_commandQueue->GetCommandList();
    auto dxDevice = m_device->DxDevice();
    uint32_t rtvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    uint32_t ssvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
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

    // Create Shader
    {
        m_vertexShaders.push_back(std::make_unique<Dx12Shader>("BasicVertex", "vs_5_1"));
        m_vertexShaders.push_back(std::make_unique<Dx12Shader>("TexVertex", "vs_5_1"));
        m_vertexShaders.push_back(std::make_unique<Dx12Shader>("RTSVertex", "vs_5_1"));

        m_pixelShaders.push_back(std::make_unique<Dx12Shader>("BasicPixel", "ps_5_1"));
        m_pixelShaders.push_back(std::make_unique<Dx12Shader>("TexPixel", "ps_5_1"));
        m_pixelShaders.push_back(std::make_unique<Dx12Shader>("RTSPixel", "ps_5_1"));

        m_computeShaders.push_back(std::make_unique<Dx12Shader>("Denoising0", "cs_5_1"));
        m_computeShaders.push_back(std::make_unique<Dx12Shader>("Denoising1", "cs_5_1"));
        m_computeShaders.push_back(std::make_unique<Dx12Shader>("Denoising2", "cs_5_1"));
        m_computeShaders.push_back(std::make_unique<Dx12Shader>("Denoising4", "cs_5_1"));
        m_computeShaders.push_back(std::make_unique<Dx12Shader>("Denoising8", "cs_5_1"));
        m_computeShaders.push_back(std::make_unique<Dx12Shader>("Denoising16", "cs_5_1"));
        m_computeShaders.push_back(std::make_unique<Dx12Shader>("DenoisingRP", "cs_5_1"));
        m_computeShaders.push_back(std::make_unique<Dx12Shader>("DenoisingReadBack", "cs_5_1"));
        m_computeShaders.push_back(std::make_unique<Dx12Shader>("InitVariance", "cs_5_1"));
    }

    // Create FrameResource Heap
    {
        // Create Descriptor Heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = m_frameCount * 6;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(dxDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
 
        D3D12_DESCRIPTOR_HEAP_DESC rayTracingHeapDesc = {};
        rayTracingHeapDesc.NumDescriptors = m_frameCount * 9 + 8;
        rayTracingHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        rayTracingHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(dxDevice->CreateDescriptorHeap(&rayTracingHeapDesc, IID_PPV_ARGS(&m_rayTracingHeap)));
       
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_CPU_DESCRIPTOR_HANDLE rayTracingTargetCpuHandle(m_rayTracingHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_GPU_DESCRIPTOR_HANDLE rayTracingTargetGpuHandle(m_rayTracingHeap->GetGPUDescriptorHandleForHeapStart());
        for(uint32_t index = 0; index < m_frameCount; index++){

            auto& frameResource = m_frameResources[index];
            frameResource.rtvHandle = rtvHandle;
            rtvHandle.Offset(1, rtvDescriptorSize);

            frameResource.gbuffer = std::make_unique<PrePass>(dxDevice, rtvHandle, rayTracingTargetCpuHandle, rayTracingTargetGpuHandle); 
            rtvHandle.Offset(5, rtvDescriptorSize);
            rayTracingTargetCpuHandle.Offset(5, ssvDescriptorSize);
            rayTracingTargetGpuHandle.Offset(5, ssvDescriptorSize);

            frameResource.uavCpuHandle[0] = rayTracingTargetCpuHandle;
            frameResource.uavGpuHandle[0] = rayTracingTargetGpuHandle;
            rayTracingTargetCpuHandle.Offset(1, ssvDescriptorSize);
            rayTracingTargetGpuHandle.Offset(1, ssvDescriptorSize);
            
            frameResource.uavCpuHandle[1] = rayTracingTargetCpuHandle;
            frameResource.uavGpuHandle[1] = rayTracingTargetGpuHandle;
            rayTracingTargetCpuHandle.Offset(1, ssvDescriptorSize);
            rayTracingTargetGpuHandle.Offset(1, ssvDescriptorSize);

            frameResource.srvCpuHandle[0] = rayTracingTargetCpuHandle;
            frameResource.srvGpuHandle[0] = rayTracingTargetGpuHandle;
            rayTracingTargetCpuHandle.Offset(1, ssvDescriptorSize);
            rayTracingTargetGpuHandle.Offset(1, ssvDescriptorSize);

            frameResource.srvCpuHandle[1] = rayTracingTargetCpuHandle;
            frameResource.srvGpuHandle[1] = rayTracingTargetGpuHandle;
            rayTracingTargetCpuHandle.Offset(1, ssvDescriptorSize);
            rayTracingTargetGpuHandle.Offset(1, ssvDescriptorSize);

        }

        for(uint32_t index = 0; index < m_frameCount; index++){
            auto& frameResource = m_frameResources[index];
            frameResource.uavCpuHandle[2] = rayTracingTargetCpuHandle;
            frameResource.uavGpuHandle[2] = rayTracingTargetGpuHandle;
        }
    
    }
}

void Dx12GraphicsManager::OnResize(uint16_t width, uint16_t height){

    m_mainConstBuffer.frameWidth = width;
    m_mainConstBuffer.frameHeight = height;

    auto& dxDevice = m_device->DxDevice();
    
    {
        for(uint32_t index = 0; index < m_frameCount; index++){
            m_frameResources[index].renderTarget.Reset();
            m_frameResources[index].rayTracingTarget[0].reset();
            m_frameResources[index].rayTracingTarget[1].reset();
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(
            m_frameCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags
        ));

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = swapChainDesc.BufferDesc.Format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


        m_varianceTarget = std::make_unique<Texture2D>(dxDevice, swapChainDesc.BufferDesc.Format, width, height, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        dxDevice->CreateUnorderedAccessView(m_varianceTarget->GetResource(), nullptr, &uavDesc, m_frameResources[0].uavCpuHandle[2]);
        for(uint32_t index = 0; index < m_frameCount; index++){
            auto& frameResource = m_frameResources[index];

            ThrowIfFailed(m_dxgiSwapChain->GetBuffer(index, IID_PPV_ARGS(frameResource.renderTarget.GetAddressOf())));
            dxDevice->CreateRenderTargetView(frameResource.renderTarget.Get(), nullptr, frameResource.rtvHandle);

            frameResource.rayTracingTarget[0] = std::make_unique<Texture2D>(dxDevice, swapChainDesc.BufferDesc.Format, width, height, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            frameResource.rayTracingTarget[1] = std::make_unique<Texture2D>(dxDevice, swapChainDesc.BufferDesc.Format, width, height, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            dxDevice->CreateUnorderedAccessView(frameResource.rayTracingTarget[0]->GetResource(), nullptr, &uavDesc, frameResource.uavCpuHandle[0]);
            dxDevice->CreateUnorderedAccessView(frameResource.rayTracingTarget[1]->GetResource(), nullptr, &uavDesc, frameResource.uavCpuHandle[1]);
            dxDevice->CreateShaderResourceView(frameResource.rayTracingTarget[0]->GetResource(), &srvDesc, frameResource.srvCpuHandle[0]);
            dxDevice->CreateShaderResourceView(frameResource.rayTracingTarget[0]->GetResource(), &srvDesc, frameResource.srvCpuHandle[1]);
            frameResource.gbuffer->ResizeBuffer(width, height, dxDevice);
        }

    }

    m_frameIndex      = (m_dxgiSwapChain->GetCurrentBackBufferIndex() + m_frameCount - 1) % m_frameCount;
}

void Dx12GraphicsManager::OnUpdate(){

    // Get New Frame Resource
    m_frameIndex = (m_frameIndex + 1) % m_frameCount;
    m_commandQueue->WaitForFenceValue(m_frameResources[m_frameIndex].fence);
    m_cmdList = m_commandQueue->GetCommandList();

    auto& currFrame = m_frameResources[m_frameIndex];
    if(currFrame.isAccelerationStructureDitry == true){
        auto& tlasDesc = currFrame.tlasDesc;
        tlasDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        tlasDesc.SourceAccelerationStructureData = currFrame.topLevelAccelerationStructure->GetGPUVirtualAddress();
        currFrame.isAccelerationStructureDitry = false;
        m_cmdList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
    }

    static std::default_random_engine e;
    static std::uniform_real_distribution<float> u(0.5f, 1000.0f);
    m_mainConstBuffer.randomSeed = GeoMath::Vector2f(u(e), u(e));

    m_frameResources[m_frameIndex].mainConst->CopyData(reinterpret_cast<uint8_t*>(&m_mainConstBuffer), sizeof(MainConstBuffer));

}

void Dx12GraphicsManager::CreatePipelineStateObject(uint64_t flag, const ComPtr<ID3D12RootSignature>& rootSignature){


        if(flag & PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER){

            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = rootSignature.Get(); 
            psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

            switch(flag & 0xF){
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_0:
                    psoDesc.CS = m_computeShaders[0]->GetByteCode();
                    break;
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_1:
                    psoDesc.CS = m_computeShaders[1]->GetByteCode();
                    break;
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_2:
                    psoDesc.CS = m_computeShaders[2]->GetByteCode();
                    break;
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_4:
                    psoDesc.CS = m_computeShaders[3]->GetByteCode();
                    break;
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_8:
                    psoDesc.CS = m_computeShaders[4]->GetByteCode();
                    break;
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_16:
                    psoDesc.CS = m_computeShaders[5]->GetByteCode();
                    break;
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_RP:
                    psoDesc.CS = m_computeShaders[6]->GetByteCode();
                    break;
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_READBACK:
                    psoDesc.CS = m_computeShaders[7]->GetByteCode();
                    break;
                case PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_VARIANCE:
                    psoDesc.CS = m_computeShaders[8]->GetByteCode();
                    break;
                default:
                    break;
            }

            ThrowIfFailed(m_device->DxDevice()->CreateComputePipelineState(
                &psoDesc, IID_PPV_ARGS(m_pipelineStateObjects[flag].GetAddressOf()))
            );

        }
        else {
            auto& psoIt = m_pipelineStateObjects.find(flag);

            if (psoIt == m_pipelineStateObjects.end()) {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

                switch (flag & 0x7000) {
                    case PipelineStateFlag::PIPELINE_STATE_RENDER_GBUFFER:
                    {
                        psoDesc.NumRenderTargets = 5;
                        psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                        psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                        psoDesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                        psoDesc.RTVFormats[3] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                        psoDesc.RTVFormats[4] = DXGI_FORMAT_R32_UINT;
                        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
                        break;
                    }
                    default:
                    {
                        psoDesc.NumRenderTargets = 1;
                        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
                        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
                        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                        break;
                    }
                }

            switch (flag & 0x7) {
                case PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_0:
                {
                    psoDesc.InputLayout = {
                       vertex0.inputLayout.data(),
                        static_cast<unsigned int>(vertex0.inputLayout.size())
                    };
                    psoDesc.VS = m_vertexShaders[0]->GetByteCode();
                    psoDesc.PS = m_pixelShaders[0]->GetByteCode();
                    break;
                }
                case PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_1:
                {
                    psoDesc.InputLayout = {
                       vertex1.inputLayout.data(),
                       static_cast<unsigned int>(vertex1.inputLayout.size())
                    };
                    psoDesc.VS = m_vertexShaders[1]->GetByteCode();
                    psoDesc.PS = m_pixelShaders[1]->GetByteCode();
                    break;
                }
                case PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_2:
                {
                    psoDesc.VS = m_vertexShaders[2]->GetByteCode();
                    psoDesc.PS = m_pixelShaders[2]->GetByteCode();
                    break;
                }
                default:
                    break;
                }

            switch (flag & 0x18) {
                case PipelineStateFlag::PIPELINE_STATE_CULL_MODE_FRONT:
                {
                    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
                    break;
                }
                case PipelineStateFlag::PIPELINE_STATE_CULL_MODE_BACK:
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

                psoDesc.pRootSignature = rootSignature.Get();
                psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
                psoDesc.RasterizerState.FrontCounterClockwise = (flag & 0x20) > 0;
                psoDesc.RasterizerState.MultisampleEnable = false;
                psoDesc.RasterizerState.DepthClipEnable = true;
                psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
                psoDesc.SampleMask = UINT_MAX;
                psoDesc.SampleDesc.Count = 1;
                psoDesc.SampleDesc.Quality = 0;
                psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

                ThrowIfFailed(m_device->DxDevice()->CreateGraphicsPipelineState(
                    &psoDesc, IID_PPV_ARGS(m_pipelineStateObjects[flag].GetAddressOf()))
                );
            }

        }

}

void Dx12GraphicsManager::SetPipelineStateFlag(uint64_t flag, uint64_t mask, bool finalFlag){

    m_currentPipelineFlag = (m_currentPipelineFlag & ~mask) | flag;

    if(finalFlag && m_currentPipelineFlag != m_cachedPipelineFlag){
        m_cmdList->SetPipelineState(m_pipelineStateObjects[m_currentPipelineFlag].Get());
        m_cachedPipelineFlag = m_currentPipelineFlag;
    }

}