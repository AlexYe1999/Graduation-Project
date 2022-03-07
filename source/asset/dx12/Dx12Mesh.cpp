#include "Dx12Mesh.hpp"

Dx12Mesh::Dx12Mesh(
    const UploadBuffer& vertexBuffer, size_t vertexCount,
    const UploadBuffer& indexBuffer, size_t indexCount,
    const Dx12SOA& SOA, const ComPtr<ID3D12Device8>& device,
    RenderResource* const renderResources
) 
    : m_indexCount(indexCount)
    , m_renderResources(renderResources)
{

    auto& varTypeData = SOA.GetVarTypeData();

    m_vertexBufferView.clear();
    m_vertexBufferView.resize(varTypeData.size());
   
    auto cmdList = renderResources->GetCommandList();
    m_vertexBuffer = std::make_unique<DefaultBuffer>(device, cmdList, vertexBuffer);
    m_indexBuffer  = std::make_unique<DefaultBuffer>(device, cmdList, indexBuffer);

    size_t addrOffset = 0;
    for(size_t index = 0; index < varTypeData.size(); index++){
        auto& data = varTypeData[index];

        size_t byteSize = data.GetSize() * vertexCount;
        auto& bufferView = m_vertexBufferView[index];

        bufferView.BufferLocation = m_vertexBuffer->GetAddress() + addrOffset;
        bufferView.SizeInBytes    = byteSize;
        bufferView.StrideInBytes  = data.GetSize();

        addrOffset += byteSize;
    }

    m_indexBufferView.BufferLocation = m_indexBuffer->GetAddress();
    m_indexBufferView.SizeInBytes    = m_indexBuffer->GetByteSize();
    m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;
    m_layoutDesc = {SOA.inputLayout.data(), static_cast<unsigned int>(SOA.inputLayout.size())};

}

void Dx12Mesh::OnRender(){

    auto cmdList = m_renderResources->GetCommandList();
    auto loadedLayoutDesc = m_renderResources->loadedLayoutDesc;

    if(loadedLayoutDesc.pInputElementDescs != m_layoutDesc.pInputElementDescs 
    || loadedLayoutDesc.NumElements != m_layoutDesc.NumElements
    ){
        if(loadedLayoutDesc.NumElements == 2){
            cmdList->SetPipelineState(m_renderResources->pipelineStateObjects[0].Get());
        }
        else{
            cmdList->SetPipelineState(m_renderResources->pipelineStateObjects[1].Get());
        }

        loadedLayoutDesc.pInputElementDescs = m_layoutDesc.pInputElementDescs;
        loadedLayoutDesc.NumElements = m_layoutDesc.NumElements;

    }

    cmdList->IASetVertexBuffers(0, m_vertexBufferView.size(), m_vertexBufferView.data());
    cmdList->IASetIndexBuffer(&m_indexBufferView);
    cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

}