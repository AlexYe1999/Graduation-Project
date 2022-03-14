#pragma once
#include "DxUtility.hpp"
#include "Texture2D.hpp"

class GBuffer{
public:
    GBuffer() {};
    ~GBuffer() {};
    virtual const ComPtr<ID3D12DescriptorHeap>& GetSRVHeap() const = 0;
    virtual std::tuple<D3D12_CPU_DESCRIPTOR_HANDLE*, uint32_t> AsRenderTarget(const ComPtr<ID3D12GraphicsCommandList2>& cmdList) = 0;
    virtual D3D12_GPU_DESCRIPTOR_HANDLE AsPixelShaderResource(const ComPtr<ID3D12GraphicsCommandList2>& cmdList) = 0;
};

class PrePass : GBuffer{
public:
    PrePass(
        const ComPtr<ID3D12Device8>& device,
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle
    ) 
        : GBuffer()
        , m_srvDescriptorSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
        , m_rtvHandles{}
    {

        auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        for(auto& handle : m_rtvHandles){
            handle = rtvHandle;
            rtvHandle.Offset(1, rtvDescriptorSize);
        }

        D3D12_DESCRIPTOR_HEAP_DESC srvDescHeapDesc = {};
        srvDescHeapDesc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvDescHeapDesc.NumDescriptors = 5;

        device->CreateDescriptorHeap(&srvDescHeapDesc, IID_PPV_ARGS(m_srvHeap.GetAddressOf()));
    }


    void ResizeBuffer(        
        const uint32_t width, const uint32_t height,
        const ComPtr<ID3D12Device8>& device
    ){

        auto srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        m_baseColor = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32G32B32A32_FLOAT, width, height);
        m_emissive  = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32G32B32A32_FLOAT, width, height);
        m_position  = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32G32B32A32_FLOAT, width, height);
        m_normal    = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32G32B32A32_FLOAT, width, height);
        m_objectID  = std::make_unique<Texture2D>(device, DXGI_FORMAT_R32_UINT, width, height);
        
        // Create RenderTarget View
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice   = 0;
            rtvDesc.Texture2D.PlaneSlice = 0;
            rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

            device->CreateRenderTargetView(m_baseColor->GetResource(), &rtvDesc, m_rtvHandles[0]);
            device->CreateRenderTargetView(m_emissive->GetResource(), &rtvDesc, m_rtvHandles[1]);
            device->CreateRenderTargetView(m_position->GetResource(), &rtvDesc, m_rtvHandles[2]);
            device->CreateRenderTargetView(m_normal->GetResource(), &rtvDesc, m_rtvHandles[3]);
            rtvDesc.Format = DXGI_FORMAT_R32_UINT;
            device->CreateRenderTargetView(m_objectID->GetResource(), &rtvDesc, m_rtvHandles[4]);
        }

        // Create Shader Resource View
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
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

            device->CreateShaderResourceView(m_emissive->GetResource(), &srvDesc, cpuSrvHandle);
            cpuSrvHandle.Offset(1, srvDescriptorSize);

            device->CreateShaderResourceView(m_position->GetResource(), &srvDesc, cpuSrvHandle);
            cpuSrvHandle.Offset(1, srvDescriptorSize);

            device->CreateShaderResourceView(m_normal->GetResource(), &srvDesc, cpuSrvHandle);
            cpuSrvHandle.Offset(1, srvDescriptorSize);

            srvDesc.Format = DXGI_FORMAT_R32_UINT;
            device->CreateShaderResourceView(m_objectID->GetResource(), &srvDesc, cpuSrvHandle);

        }

    }

    virtual const ComPtr<ID3D12DescriptorHeap>& GetSRVHeap() const override{
        return m_srvHeap;
    }

    virtual std::tuple<D3D12_CPU_DESCRIPTOR_HANDLE*, uint32_t> AsRenderTarget(const ComPtr<ID3D12GraphicsCommandList2>& cmdList) override{
        m_baseColor->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_emissive->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_position->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_normal->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_objectID->ChangeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        return {m_rtvHandles.data(), m_rtvHandles.size()};
    }

    virtual D3D12_GPU_DESCRIPTOR_HANDLE AsPixelShaderResource(const ComPtr<ID3D12GraphicsCommandList2>& cmdList) override{
        m_baseColor->ChangeState(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_emissive->ChangeState(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_position->ChangeState(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_normal->ChangeState(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_objectID->ChangeState(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        return m_srvHeap->GetGPUDescriptorHandleForHeapStart();
    }

protected:
    std::unique_ptr<Texture2D>                 m_baseColor;
    std::unique_ptr<Texture2D>                 m_emissive;
    std::unique_ptr<Texture2D>                 m_position; 
    std::unique_ptr<Texture2D>                 m_normal;
    std::unique_ptr<Texture2D>                 m_objectID;

    uint32_t                                   m_srvDescriptorSize;
    ComPtr<ID3D12DescriptorHeap>               m_srvHeap;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 5> m_rtvHandles;
};