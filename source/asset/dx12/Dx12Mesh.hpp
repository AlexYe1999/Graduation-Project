#pragma once
#include "Mesh.hpp"
#include "DefaultBuffer.hpp"
#include "DxUtility.hpp"
#include "Dx12Material.hpp"
#include "Dx12Struct.hpp"
#include "GraphicsManager.hpp"

class Dx12Mesh : public Mesh{
public:
    Dx12Mesh(
        UploadBuffer& vertexBuffer, size_t vertexCount, 
        UploadBuffer& indexBuffer, size_t indexCount,
        const PipelineStateFlag flag, const std::shared_ptr<Material>& material,
        const ComPtr<ID3D12Device8>& device
    );
    
    virtual void OnRender() override;

protected:
    size_t                                m_indexCount;
    std::unique_ptr<DefaultBuffer>        m_vertexBuffer;
    std::unique_ptr<DefaultBuffer>        m_indexBuffer;

    std::vector<D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW               m_indexBufferView;

    uint64_t                              m_meshFlag;
    Dx12GraphicsManager* const            m_graphicsMgr;
};