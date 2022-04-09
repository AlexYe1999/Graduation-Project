#include "Dx12Mesh.hpp"

Dx12Mesh::Dx12Mesh(
    UploadBuffer& vertexBuffer, size_t vertexCount,
    UploadBuffer& indexBuffer, size_t indexCount,
    const PipelineStateFlag flag, const std::shared_ptr<Material>& material,
    const ComPtr<ID3D12Device8>& device
) 
    : Mesh(material)
    , m_indexCount(indexCount)
    , m_meshFlag(flag)
    , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
{

    auto GetSOA = [](uint64_t flag) -> Dx12SOA&{
        switch(flag){
            case Utility::Predef::PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_0:
                return vertex0;
            case Utility::Predef::PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_1:
                return vertex1;
            default:
                return vertex1;
                break;
        }
    };

    auto& varTypeData = GetSOA(m_meshFlag).GetVarTypeData();

    m_vertexBufferView.clear();
    m_vertexBufferView.resize(varTypeData.size());
   
    auto cmdList = m_graphicsMgr->GetTempCommandList();
    m_vertexBuffer = std::make_unique<DefaultBuffer>(device, cmdList, vertexBuffer);
    m_indexBuffer  = std::make_unique<DefaultBuffer>(device, cmdList, indexBuffer);

    size_t addrOffset = 0;
    for(size_t index = 0; index < varTypeData.size(); index++){
        auto& data = varTypeData[index];

        size_t byteSize = data.GetSize() * vertexCount;
        auto& bufferView = m_vertexBufferView[index];

        bufferView.BufferLocation = m_vertexBuffer->GetGpuVirtualAddress() + addrOffset;
        bufferView.SizeInBytes    = byteSize;
        bufferView.StrideInBytes  = data.GetSize();

        addrOffset += byteSize;
    }

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGpuVirtualAddress();
    m_indexBufferView.SizeInBytes    = m_indexBuffer->GetByteSize();
    m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;
    
    m_graphicsMgr->ExecuteCommandList(cmdList);
}

void Dx12Mesh::OnRender(){

    m_material->OnRender();
    m_graphicsMgr->SetPipelineStateFlag(m_meshFlag, 0x7, true);

    auto cmdList = m_graphicsMgr->GetCommandList();
    cmdList->IASetVertexBuffers(0, m_vertexBufferView.size(), m_vertexBufferView.data());
    cmdList->IASetIndexBuffer(&m_indexBufferView);
    cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

}