#pragma once
#include "GpuResource.hpp"
#include "DxUtility.hpp"

class Texture2D : public GpuResource{
public:
    Texture2D(
        const ComPtr<ID3D12Device8>& device, const DXGI_FORMAT format,
        const uint32_t width, const uint32_t height, 
        D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE
    ) : GpuResource()
      , m_format(format)
      , m_width(width)
      , m_height(height)
    {
         
        D3D12_CLEAR_VALUE clearValue = {format, 0.0f, 0.0f, 0.0f, 0.0f};
        m_usageState = flag == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : m_usageState;

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0, flag), 
            m_usageState, flag == D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? &clearValue : nullptr,
            IID_PPV_ARGS(m_resource.GetAddressOf()))
		);

    }

    Texture2D(
        const ComPtr<ID3D12Device8>& device, 
        const ComPtr<ID3D12GraphicsCommandList2>& cmdList, UploadBuffer& uploadBuffer,
        const DXGI_FORMAT format, const uint32_t width, const uint32_t height
    ) : GpuResource()
      , m_format(format)
      , m_width(width)
      , m_height(height)
    {

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
                format, width, height, 1, 1, 1, 0,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
            ), m_usageState, nullptr, IID_PPV_ARGS(m_resource.GetAddressOf()))
		);

        ChangeState(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_SUBRESOURCE_FOOTPRINT subResFootPrint = {};
        subResFootPrint.Width  = width;
        subResFootPrint.Height = height;
        subResFootPrint.Depth  = 1;
        subResFootPrint.Format = format;
        subResFootPrint.RowPitch = Utility::CalcAlignment<D3D12_TEXTURE_DATA_PITCH_ALIGNMENT>(width*4);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT pitchedDesc;
        pitchedDesc.Offset    = 0;
        pitchedDesc.Footprint = subResFootPrint;

        cmdList->CopyTextureRegion(
            &CD3DX12_TEXTURE_COPY_LOCATION( m_resource.Get(), 0 ), 0, 0, 0, 
            &CD3DX12_TEXTURE_COPY_LOCATION( uploadBuffer.GetResource(), pitchedDesc), nullptr
        );

        ChangeState(cmdList, D3D12_RESOURCE_STATE_COMMON);
    }

    void ChangeState(const ComPtr<ID3D12GraphicsCommandList2>& cmdList, D3D12_RESOURCE_STATES state){
        if(m_transitioningState != state){
            cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                m_resource.Get(), m_transitioningState, state
            ));
            m_transitioningState = state;
        }
    }

protected:
    DXGI_FORMAT m_format;
    uint32_t    m_width;
    uint32_t    m_height;
};