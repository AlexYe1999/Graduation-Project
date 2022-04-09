#pragma once
#include "GpuBuffer.hpp"
#include "UploadBuffer.hpp"

class DefaultBuffer : public GpuBuffer{
public:
    DefaultBuffer(
        const ComPtr<ID3D12Device8>& device,
        const ComPtr<ID3D12GraphicsCommandList2>& cmdList,
        UploadBuffer& uploadBuffer, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    )  
        : GpuBuffer(uploadBuffer.GetByteSize(), 1, state)
    {

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(m_byteSize), D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(m_resource.GetAddressOf()))
        );

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(),
            D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
        ));

        cmdList->CopyBufferRegion(m_resource.Get(), 0, uploadBuffer.GetResource(), 0, m_byteSize);

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, m_usageState
        ));

        m_GpuVirtualAddress = m_resource->GetGPUVirtualAddress();
    }

    DefaultBuffer(
        const ComPtr<ID3D12Device8>& device,
        const ComPtr<ID3D12GraphicsCommandList2>& cmdList,
        const uint32_t bufferByteSize, std::vector<UploadBuffer>& uploadBuffers,
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    )  
        : GpuBuffer(bufferByteSize, 1, state)
    {

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(m_byteSize), D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(m_resource.GetAddressOf()))
        );

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(),
            D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
        ));

        size_t dstOffset = 0;
        for(auto& src : uploadBuffers){
            cmdList->CopyBufferRegion(m_resource.Get(), dstOffset, src.GetResource(), 0, src.GetByteSize());
            dstOffset += src.GetByteSize();
        }

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, m_usageState
        ));

        m_GpuVirtualAddress = m_resource->GetGPUVirtualAddress();
    }

    DefaultBuffer(DefaultBuffer&&) = delete;
    DefaultBuffer(const DefaultBuffer&) = delete;

};
