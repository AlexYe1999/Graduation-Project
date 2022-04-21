#pragma once
#include "DxUtility.hpp"
#include "Texture2D.hpp"

class GBuffer{
public:
    GBuffer() {};
    ~GBuffer() {};
    virtual const CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const = 0;
    virtual std::tuple<D3D12_CPU_DESCRIPTOR_HANDLE*, uint32_t> AsRenderTarget(const ComPtr<ID3D12GraphicsCommandList2>& cmdList) = 0;
    virtual D3D12_GPU_DESCRIPTOR_HANDLE AsShaderResource(
        const ComPtr<ID3D12GraphicsCommandList2>& cmdList, 
        const D3D12_RESOURCE_STATES state
    ) = 0;
};

class PrePass : GBuffer{
public:
    PrePass(
        const ComPtr<ID3D12Device8>& device,
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvCPUHandle,
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvGPUHandle
    )
        : GBuffer()
        , m_srvDescriptorSize{device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)}
        , m_rtvHandles{rtvHandle}
        , m_srvCPUHandle{srvCPUHandle}
        , m_srvGPUHandle{srvGPUHandle}
    {

        auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        for(auto& handle : m_rtvHandles){
            handle = rtvHandle;
            rtvHandle.Offset(1, rtvDescriptorSize);
        }

    }

    void ResizeBuffer(
        const uint32_t width, const uint32_t height,
        const ComPtr<ID3D12Device8>& device
    ){

        auto srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        m_baseColor = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        m_position  = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        m_normal    = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        m_misc      = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        m_objectID  = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32_UINT, width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        
        // Create RenderTarget View
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice   = 0;
            rtvDesc.Texture2D.PlaneSlice = 0;

            rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            device->CreateRenderTargetView(m_baseColor->GetResource(), &rtvDesc, m_rtvHandles[0]);
            device->CreateRenderTargetView(m_misc->GetResource(), &rtvDesc, m_rtvHandles[1]);
            device->CreateRenderTargetView(m_position->GetResource(), &rtvDesc, m_rtvHandles[2]);
            device->CreateRenderTargetView(m_normal->GetResource(), &rtvDesc, m_rtvHandles[3]);

            rtvDesc.Format = DXGI_FORMAT_R32_UINT;
            device->CreateRenderTargetView(m_objectID->GetResource(), &rtvDesc, m_rtvHandles[4]);
        }

        // Create Shader Resource View
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle(m_srvCPUHandle);
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            device->CreateShaderResourceView(m_baseColor->GetResource(), &srvDesc, cpuSrvHandle);
            cpuSrvHandle.Offset(1, srvDescriptorSize);
            device->CreateShaderResourceView(m_misc->GetResource(), &srvDesc, cpuSrvHandle);
            cpuSrvHandle.Offset(1, srvDescriptorSize);
            device->CreateShaderResourceView(m_position->GetResource(), &srvDesc, cpuSrvHandle);
            cpuSrvHandle.Offset(1, srvDescriptorSize);
            device->CreateShaderResourceView(m_normal->GetResource(), &srvDesc, cpuSrvHandle);
            cpuSrvHandle.Offset(1, srvDescriptorSize);

            srvDesc.Format = DXGI_FORMAT_R32_UINT;
            device->CreateShaderResourceView(m_objectID->GetResource(), &srvDesc, cpuSrvHandle);

        }

    }

    virtual const CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const override{
        return m_srvGPUHandle;
    }

    virtual std::tuple<D3D12_CPU_DESCRIPTOR_HANDLE*, uint32_t> AsRenderTarget(const ComPtr<ID3D12GraphicsCommandList2>& cmdList) override{

        static float clearValue[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        m_baseColor->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_position->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_normal->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_misc->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_objectID->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        cmdList->ClearRenderTargetView(m_rtvHandles[0], clearValue, 0, nullptr);
        cmdList->ClearRenderTargetView(m_rtvHandles[1], clearValue, 0, nullptr);
        cmdList->ClearRenderTargetView(m_rtvHandles[2], clearValue, 0, nullptr);
        cmdList->ClearRenderTargetView(m_rtvHandles[3], clearValue, 0, nullptr);
        cmdList->ClearRenderTargetView(m_rtvHandles[4], clearValue, 0, nullptr);

        return {m_rtvHandles.data(), m_rtvHandles.size()};
    }

    virtual D3D12_GPU_DESCRIPTOR_HANDLE AsShaderResource(
        const ComPtr<ID3D12GraphicsCommandList2>& cmdList,
        const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    ) override{
        m_baseColor->ChangeState(cmdList, state);
        m_position->ChangeState(cmdList, state);
        m_normal->ChangeState(cmdList, state);
        m_misc->ChangeState(cmdList, state);
        m_objectID->ChangeState(cmdList, state);

        return m_srvGPUHandle;
    }

protected:
    std::unique_ptr<Texture2D>                 m_baseColor;
    std::unique_ptr<Texture2D>                 m_position; 
    std::unique_ptr<Texture2D>                 m_normal;
    std::unique_ptr<Texture2D>                 m_misc;
    std::unique_ptr<Texture2D>                 m_objectID;

    uint32_t                                   m_srvDescriptorSize;
    CD3DX12_CPU_DESCRIPTOR_HANDLE              m_srvCPUHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE              m_srvGPUHandle;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 5> m_rtvHandles;
};