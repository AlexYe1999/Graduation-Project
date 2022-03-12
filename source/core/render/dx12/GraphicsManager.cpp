#include "GraphicsManager.hpp"
#include "Material.hpp"

Dx12GraphicsManager::Dx12GraphicsManager()
    : m_frameIndex(0)
    , m_frameCount(0)
    , m_cachedPipelineFlag(PipelineStateFlag::PIPELINE_STATE_INITIAL_FLAG)
    , m_currentPipelineFlag(PipelineStateFlag::PIPELINE_STATE_INITIAL_FLAG)
{}

void Dx12GraphicsManager::OnInit(
    HWND hWnd, uint8_t frameCount, uint16_t width, uint16_t height
){
    m_frameCount     = frameCount;
    m_device         = std::make_unique<Device>();
    m_commandQueue   = std::make_unique<CommandQueue>(m_device->DxDevice());
    m_frameResources = std::make_unique<FrameResource[]>(frameCount);
    m_cmdList        = m_commandQueue->GetCommandList();

    auto dxDevice = m_device->DxDevice();
    
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
        m_vertexShaders.push_back(std::make_unique<Dx12Shader>("BasicVertexShader", "vs_5_1"));
        m_vertexShaders.push_back(std::make_unique<Dx12Shader>("TexVertexShader", "vs_5_1"));
        m_pixelShaders.push_back(std::make_unique<Dx12Shader>("BasicPixelShader", "ps_5_1"));
        m_pixelShaders.push_back(std::make_unique<Dx12Shader>("TexPixelShader", "ps_5_1"));
    }

}

void Dx12GraphicsManager::OnResize(uint16_t width, uint16_t height){

    m_commandQueue->Flush();

    auto dxDevice = m_device->DxDevice();

    {

        for(uint32_t index = 0; index < m_frameCount; index++){
            m_frameResources[index].renderTarget.Reset();
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(
            m_frameCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags
        ));

        for(uint32_t index = 0; index < m_frameCount; index++){
            auto& frameResource = m_frameResources[index];

            frameResource.renderTarget.Reset();
            ThrowIfFailed(m_dxgiSwapChain->GetBuffer(index, IID_PPV_ARGS(frameResource.renderTarget.GetAddressOf())));
            dxDevice->CreateRenderTargetView(frameResource.renderTarget.Get(), nullptr, frameResource.rtvHandle);

            frameResource.gbuffer->ResizeBuffer(width, height, dxDevice);
        }

    }


    m_frameIndex = (m_dxgiSwapChain->GetCurrentBackBufferIndex() + m_frameCount - 1) % m_frameCount;

}

void Dx12GraphicsManager::CreatePipelineStateObject(uint64_t flag, const ComPtr<ID3D12RootSignature>& rootSignature){

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

        psoDesc.pRootSignature = rootSignature.Get();
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