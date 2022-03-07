#pragma once
#include "Mesh.hpp"
#include "DefaultBuffer.hpp"
#include "Dx12Texture.hpp"
#include "Dx12Struct.hpp"
#include "RenderResource.hpp"

class Dx12Mesh : public Mesh{
public:
    Dx12Mesh(
        const UploadBuffer& vertexBuffer, size_t vertexCount, 
        const UploadBuffer& indexBuffer, size_t indexCount,
        const Dx12SOA& SOA, const ComPtr<ID3D12Device8>& device,
        RenderResource* const renderResources
    );
    
    virtual void OnRender() override;

protected:
    size_t                                m_indexCount;
    std::unique_ptr<DefaultBuffer>        m_vertexBuffer;
    std::unique_ptr<DefaultBuffer>        m_indexBuffer;

    std::vector<D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW               m_indexBufferView;
    D3D12_INPUT_LAYOUT_DESC               m_layoutDesc;

    RenderResource* const                 m_renderResources;
};