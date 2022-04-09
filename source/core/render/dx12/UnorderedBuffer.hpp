#pragma once
#include "GpuBuffer.hpp"

class UnorderedBuffer : public GpuBuffer{
public:

    UnorderedBuffer(const ComPtr<ID3D12Device8>& device, uint32_t elementNum, uint32_t elementSize)   
		: GpuBuffer(elementNum, elementSize, D3D12_RESOURCE_STATE_GENERIC_READ)
    {
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_byteSize), m_usageState,
			nullptr, IID_PPV_ARGS(m_resource.GetAddressOf()))
		);

		m_GpuVirtualAddress = m_resource->GetGPUVirtualAddress();
    }


};