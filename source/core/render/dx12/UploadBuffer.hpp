#pragma once
#include "GpuBuffer.hpp"

class UploadBuffer : public GpuBuffer{
public:
    UploadBuffer(const ComPtr<ID3D12Device8>& device, uint32_t elementNum, uint32_t elementSize)   
		: GpuBuffer(elementNum, elementSize, D3D12_RESOURCE_STATE_GENERIC_READ)
	{

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_byteSize), m_usageState,
			nullptr, IID_PPV_ARGS(m_resource.GetAddressOf()))
		);

		m_GpuVirtualAddress = m_resource->GetGPUVirtualAddress();
	}

    void ChangeState(const ComPtr<ID3D12GraphicsCommandList2>& cmdList, D3D12_RESOURCE_STATES state){  
		if(m_transitioningState != state){
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				m_resource.Get(), m_transitioningState, state
			));
			m_transitioningState = state;
		}
    }
	
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