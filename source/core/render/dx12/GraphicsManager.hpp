#pragma once
#include "CommandQueue.hpp"
#include "DefaultBuffer.hpp"
#include "Device.hpp"
#include "Dx12Shader.hpp"
#include "Dx12Struct.hpp"
#include "DxUtility.hpp"


struct FrameResource{
    FrameResource() 
        : fence(0)
    {}

    uint64_t                      fence;
    ComPtr<ID3D12Resource>        renderTarget;
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;

    std::unique_ptr<UploadBuffer> mainConst;
    std::unique_ptr<UploadBuffer> objectConst;

};

using Utility::Predef::PipelineStateFlag;
class Dx12GraphicsManager{
public:
    static Dx12GraphicsManager* GetInstance() {
        s_instance = s_instance == nullptr ? new Dx12GraphicsManager() : s_instance;
        return s_instance;
    }

    void OnInit(uint8_t frameCount, uint16_t width, uint16_t height, HWND hWnd);
    void OnUpdate(GeoMath::Vector4f& backgroundColor);
    void OnResize(uint16_t width, uint16_t height);
    void OnRender(){
        auto& frameResource = m_frameResources[m_frameIndex];

        m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            frameResource.renderTarget.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
        ));

        frameResource.fence = m_commandQueue->ExecuteCommandList(m_cmdList);
        m_dxgiSwapChain->Present(1, 0);
    }


    ComPtr<ID3D12Device8> GetDevice() const { return m_device->DxDevice(); }
    ComPtr<ID3D12GraphicsCommandList2> GetCommandList() const { return m_cmdList; }
    
    uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2>& cmdList) const {
        return m_commandQueue->ExecuteCommandList(cmdList);
    }

    void Flush() const { m_commandQueue->Flush(); }

    void CreateRenderResource(
        size_t numObject, 
        size_t numMat, const UploadBuffer& matResource,
        size_t numTex
    );

    FrameResource& GetFrameResource() const { return m_frameResources[m_frameIndex]; }
    uint8_t GetFrameCount() const { return m_frameCount; };

    void CreatePipelineStateObject(uint64_t flag);
    void SetPipelineStateFlag(uint64_t flag, uint64_t mask, bool finalFlag);

    Dx12GraphicsManager(const Dx12GraphicsManager&) = delete;
    Dx12GraphicsManager(Dx12GraphicsManager&&) = delete;
    Dx12GraphicsManager& operator=(const Dx12GraphicsManager&) = delete;
    Dx12GraphicsManager& operator=(Dx12GraphicsManager&&) = delete;

protected:
    inline static Dx12GraphicsManager* s_instance = nullptr;

    HWND                               m_wnd;
    uint8_t                            m_frameIndex;
    uint8_t                            m_frameCount;
    uint32_t                           m_rtvDescriptorSize;
    uint32_t                           m_dsvDescriptorSize;
    uint32_t                           m_ssvDescriptorSize;

    uint32_t                           m_numConstPerFrame;
    uint32_t                           m_matConstViewHeapOffset;

    std::unique_ptr<Device>            m_device;
    std::unique_ptr<CommandQueue>      m_commandQueue;
    std::unique_ptr<FrameResource[]>   m_frameResources;

    ComPtr<IDXGISwapChain3>            m_dxgiSwapChain;
    ComPtr<ID3D12GraphicsCommandList2> m_cmdList;

    using Shaders = std::vector<std::unique_ptr<Dx12Shader>>;
    using PipelineStateObjects = std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>>;

    D3D12_VIEWPORT                     m_viewport;
    D3D12_RECT                         m_scissors;

    Shaders                            m_vertexShaders;
    Shaders                            m_geomertyShaders;
    Shaders                            m_pixelShaders;

    uint64_t                           m_cachedPipelineFlag;
    uint64_t                           m_currentPipelineFlag;
    PipelineStateObjects               m_pipelineStateObjects;
    ComPtr<ID3D12RootSignature>        m_rootSignatureDeferred;

    ComPtr<ID3D12Resource>             m_depthStencil;
    std::unique_ptr<DefaultBuffer>     m_matConstBuffer;

    ComPtr<ID3D12DescriptorHeap>       m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_svvHeap;

    CD3DX12_CPU_DESCRIPTOR_HANDLE      m_dsvHandle;

    Dx12GraphicsManager();
    ~Dx12GraphicsManager(){ m_commandQueue->Flush(); }

};