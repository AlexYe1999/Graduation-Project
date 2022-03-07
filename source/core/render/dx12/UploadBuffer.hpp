#pragma once
#include "Buffer.hpp"

class UploadBuffer : public Buffer{
public:
    UploadBuffer(const ComPtr<ID3D12Device8>& device, size_t byteSize)   
		: Buffer(byteSize, D3D12_RESOURCE_STATE_GENERIC_READ)
	{

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize), m_resourceState,
			nullptr, IID_PPV_ARGS(m_resource.GetAddressOf()))
		);

	}

    UploadBuffer(UploadBuffer&&)      = default;
	UploadBuffer(const UploadBuffer&) = delete;

    void CopyData(const uint8_t* data, size_t byteSize, size_t offset = 0) const {
		void* mappedData;

		D3D12_RANGE range;
		range.Begin = offset;
		range.End = offset + byteSize;
	
		ThrowIfFailed(m_resource->Map(0, &range, reinterpret_cast<void**>(&mappedData)));
		memcpy(reinterpret_cast<uint8_t*>(mappedData) + offset, data, byteSize);
		m_resource->Unmap(0, &range);
	}
};