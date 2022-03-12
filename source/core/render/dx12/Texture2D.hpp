#pragma once
#include "Resource.hpp"
#include "DxUtility.hpp"

class Texture2D : public Resource{
public:
    Texture2D(
        const ComPtr<ID3D12Device8>& device, const DXGI_FORMAT format,
        const uint32_t width, const uint32_t height
    ) : Resource()
      , m_format(format)
      , m_width(width)
      , m_height(height)
    {

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
                format, width, height, 1, 0, 1, 0,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
            ), m_resourceState, nullptr, IID_PPV_ARGS(m_resource.GetAddressOf()))
		);

    }

    void ChangeState(const ComPtr<ID3D12GraphicsCommandList2>& cmdList, D3D12_RESOURCE_STATES state){
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(), m_resourceState, state
        ));
        m_resourceState = state;
    }

protected:
    DXGI_FORMAT m_format;
    uint32_t    m_width;
    uint32_t    m_height;

};