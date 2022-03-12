#pragma once
#include "CommandQueue.hpp"
#include "DefaultBuffer.hpp"
#include "Device.hpp"
#include "Dx12Shader.hpp"
#include "Dx12Struct.hpp"
#include "DxUtility.hpp"
#include "GBuffer.hpp"

struct FrameResource{
    FrameResource() 
        : fence(0)
    {}

    uint64_t                      fence;
    ComPtr<ID3D12Resource>        renderTarget;
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;

    std::unique_ptr<UploadBuffer> mainConst;
    std::unique_ptr<UploadBuffer> objectConst;
    std::unique_ptr<PrePass>      gbuffer;
};

using Utility::Predef::PipelineStateFlag;
class Dx12GraphicsManager{
public:
    static Dx12GraphicsManager* GetInstance() {
        s_instance = s_instance == nullptr ? new Dx12GraphicsManager() : s_instance;
        return s_instance;
    }

    void OnInit(HWND hWnd, uint8_t frameCount, uint16_t width, uint16_t height);
    void OnResize(uint16_t width, uint16_t height);
    
    void OnUpdate(){
        // Get New Frame Resource
        m_frameIndex = (m_frameIndex + 1) % m_frameCount;
        m_commandQueue->WaitForFenceValue(m_frameResources[m_frameIndex].fence);

        m_cmdList = m_commandQueue->GetCommandList();
    }

    ComPtr<ID3D12Device8> GetDevice() const { return m_device->DxDevice(); }
    ComPtr<IDXGISwapChain3> GetSwapChain() const{ return m_dxgiSwapChain; }
    ComPtr<ID3D12GraphicsCommandList2> GetCommandList() const { return m_cmdList; }

    uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2>& cmdList) const {
        return m_commandQueue->ExecuteCommandList(cmdList);
    }

    void Flush() const { m_commandQueue->Flush(); }

    FrameResource& GetFrameResource() const { return m_frameResources[m_frameIndex]; }
    FrameResource& GetFrameResource(const uint32_t frameIndex) const { return m_frameResources[frameIndex]; }
    uint8_t GetFrameCount() const { return m_frameCount; };

    void CreatePipelineStateObject(uint64_t flag, const ComPtr<ID3D12RootSignature>& rootSignature);
    void SetPipelineStateFlag(uint64_t flag, uint64_t mask, bool finalFlag);
    
    Dx12GraphicsManager(const Dx12GraphicsManager&) = delete;
    Dx12GraphicsManager(Dx12GraphicsManager&&) = delete;
    Dx12GraphicsManager& operator=(const Dx12GraphicsManager&) = delete;
    Dx12GraphicsManager& operator=(Dx12GraphicsManager&&) = delete;

protected:
    inline static Dx12GraphicsManager* s_instance = nullptr;

    uint8_t                            m_frameIndex;
    uint8_t                            m_frameCount;

    std::unique_ptr<Device>            m_device;
    std::unique_ptr<CommandQueue>      m_commandQueue;
    std::unique_ptr<FrameResource[]>   m_frameResources;

    ComPtr<IDXGISwapChain3>            m_dxgiSwapChain;
    ComPtr<ID3D12GraphicsCommandList2> m_cmdList;

    using Shaders = std::vector<std::unique_ptr<Dx12Shader>>;
    using PipelineStateObjects = std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>>;

    Shaders                            m_vertexShaders;
    Shaders                            m_geomertyShaders;
    Shaders                            m_pixelShaders;

    uint64_t                           m_cachedPipelineFlag;
    uint64_t                           m_currentPipelineFlag;
    PipelineStateObjects               m_pipelineStateObjects;

    Dx12GraphicsManager();
    ~Dx12GraphicsManager(){ m_commandQueue->Flush(); }

};