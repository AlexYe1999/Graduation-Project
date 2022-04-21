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
        : fence{0}
        , isAccelerationStructureDitry{true}
        , tlasDesc{}
    {}

    uint64_t                      fence;
    bool                          isAccelerationStructureDitry;

    std::unique_ptr<UploadBuffer> mainConst;
    std::unique_ptr<UploadBuffer> objectConst;
    std::unique_ptr<PrePass>      gbuffer;
    
    ComPtr<ID3D12Resource>        renderTarget;
    D3D12_CPU_DESCRIPTOR_HANDLE   rtvHandle;

    // Ray Tracing Resources
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc;
    ComPtr<ID3D12Resource>        scrachData;
    ComPtr<ID3D12Resource>        topLevelAccelerationStructure;
    std::unique_ptr<UploadBuffer> rayTraceInstanceDesc;

    std::unique_ptr<Texture2D>    rayTracingTarget[2];
    D3D12_CPU_DESCRIPTOR_HANDLE   uavCpuHandle[3];
    D3D12_GPU_DESCRIPTOR_HANDLE   uavGpuHandle[3];
    D3D12_CPU_DESCRIPTOR_HANDLE   srvCpuHandle[2];
    D3D12_GPU_DESCRIPTOR_HANDLE   srvGpuHandle[2];
};

struct DxilLibrary{
    DxilLibrary(ComPtr<ID3DBlob> pBlob, const wchar_t* entryPoint[], uint32_t entryPointCount)
        : pShaderBlob(pBlob)
    {
        stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        stateSubobject.pDesc = &dxilLibDesc;

        dxilLibDesc = {};
        exportDesc.resize(entryPointCount);
        exportName.resize(entryPointCount);
        if (pBlob){
            dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
            dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
            dxilLibDesc.NumExports = entryPointCount;
            dxilLibDesc.pExports = exportDesc.data();

            for (uint32_t i = 0; i < entryPointCount; i++){
                exportName[i] = entryPoint[i];
                exportDesc[i].Name = exportName[i].c_str();
                exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
                exportDesc[i].ExportToRename = nullptr;
            }
        }
    };

    DxilLibrary() : DxilLibrary(nullptr, nullptr, 0) {}

    D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
    D3D12_STATE_SUBOBJECT stateSubobject{};
    ComPtr<ID3DBlob> pShaderBlob;
    std::vector<D3D12_EXPORT_DESC> exportDesc;
    std::vector<std::wstring> exportName;
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
    void OnUpdate();

    void OnRender(){
        m_frameResources[m_frameIndex].fence = m_commandQueue->ExecuteCommandList(m_cmdList);
        m_dxgiSwapChain->Present(1, 0);
    }

    ComPtr<ID3D12Device8> GetDevice() const { return m_device->DxDevice(); }
    ComPtr<ID3D12GraphicsCommandList4> GetCommandList() const { return m_cmdList; }
    ComPtr<ID3D12GraphicsCommandList4> GetTempCommandList() {
        return m_commandQueue->GetCommandList();
    }

    uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList4>& cmdList) const {
        return m_commandQueue->ExecuteCommandList(cmdList);
    }

    void Flush() const { m_commandQueue->Flush(); }

    FrameResource& GetFrameResource() const { return m_frameResources[m_frameIndex]; }
    FrameResource& GetFrameResource(uint32_t frameIndex) const { return m_frameResources[frameIndex]; }
    FrameResource& GetPreFrameResource() const { return m_frameResources[(m_frameIndex+m_frameCount-1)%m_frameCount]; }
    
    uint8_t           GetFrameCount() const { return m_frameCount; };
    GeoMath::Vector2f GetFrameResolution() const{ return m_frameResolution; };

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetTexCpuHandle() const{
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(
            m_rayTracingHeap->GetCPUDescriptorHandleForHeapStart(), m_frameCount * 9 + 1, 
            m_device->DxDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        );
    };

    CD3DX12_GPU_DESCRIPTOR_HANDLE GetTexGpuHandle() const{
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(
            m_rayTracingHeap->GetGPUDescriptorHandleForHeapStart(), m_frameCount * 9 + 1, 
            m_device->DxDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        );
    };

    ComPtr<ID3D12DescriptorHeap> GetRTHeap() const{
        return m_rayTracingHeap;
    };

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

    GeoMath::Vector2f                  m_frameResolution;

    std::unique_ptr<Device>            m_device;
    std::unique_ptr<CommandQueue>      m_commandQueue;
    std::unique_ptr<FrameResource[]>   m_frameResources;

    ComPtr<IDXGISwapChain3>            m_dxgiSwapChain;
    ComPtr<ID3D12GraphicsCommandList4> m_cmdList;

    using Shaders = std::vector<std::unique_ptr<Dx12Shader>>;
    using PipelineStateObjects = std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>>;

    Shaders                            m_vertexShaders;
    Shaders                            m_geometryShaders;
    Shaders                            m_pixelShaders;
    Shaders                            m_computeShaders;

    uint64_t                           m_cachedPipelineFlag;
    uint64_t                           m_currentPipelineFlag;
    PipelineStateObjects               m_pipelineStateObjects;

    ComPtr<ID3D12DescriptorHeap>       m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_rayTracingHeap;

    std::unique_ptr<Texture2D>         m_varianceTarget;
    Dx12GraphicsManager();
    ~Dx12GraphicsManager(){ m_commandQueue->Flush(); }

};