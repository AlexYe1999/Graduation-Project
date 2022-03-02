#include "Dx12Mesh.hpp"

Dx12Mesh::Dx12Mesh(
    const UploadBuffer& vertexBuffer, size_t vertexCount,
    const UploadBuffer& indexBuffer, size_t indexCount,
    const std::vector<rtti::VarTypeData>& varTypeData,
    const ComPtr<ID3D12Device8>& device,
    RenderResource* const renderResources
) 
    : m_indexCount(indexCount)
    , m_renderResources(renderResources)
{

    auto GetDxFormat = [&](rtti::VarTypeData typeData)->DXGI_FORMAT{

        switch(typeData.scale){
            case rtti::VarTypeData::ScaleType::Float:
                switch(typeData.dimension){
                    case 1:
                        return DXGI_FORMAT_R32_FLOAT;
                    case 2:
                        return DXGI_FORMAT_R32G32_FLOAT;
                    case 3:
                        return DXGI_FORMAT_R32G32B32_FLOAT;
                    case 4:
                        return DXGI_FORMAT_R32G32B32A32_FLOAT;
                    default:
                        return DXGI_FORMAT_UNKNOWN;
                }
            case rtti::VarTypeData::ScaleType::Int:
                switch(typeData.dimension){
					case 1:
						return DXGI_FORMAT_R32_SINT;
					case 2:
						return DXGI_FORMAT_R32G32_SINT;
					case 3:
						return DXGI_FORMAT_R32G32B32_SINT;
					case 4:
						return DXGI_FORMAT_R32G32B32A32_SINT;
					default:
						return DXGI_FORMAT_UNKNOWN;
                }
            case rtti::VarTypeData::ScaleType::UInt:
                switch(typeData.dimension){
					case 1:
						return DXGI_FORMAT_R32_UINT;
					case 2:
						return DXGI_FORMAT_R32G32_UINT;
					case 3:
						return DXGI_FORMAT_R32G32B32_UINT;
					case 4:
						return DXGI_FORMAT_R32G32B32A32_UINT;
					default:
						return DXGI_FORMAT_UNKNOWN;
                }

            default:
                return DXGI_FORMAT_UNKNOWN;
        }
    };

    m_elementDesc.clear();
    m_vertexBufferView.clear();
    m_elementDesc.resize(varTypeData.size());
    m_vertexBufferView.resize(varTypeData.size());
   
    auto cmdList = renderResources->GetCommandList();
    m_vertexBuffer = std::make_unique<DefaultBuffer>(device, cmdList, vertexBuffer);
    m_indexBuffer  = std::make_unique<DefaultBuffer>(device, cmdList, indexBuffer);

    size_t addrOffset = 0;
    for(size_t index = 0; index < varTypeData.size(); index++){
        auto& data = varTypeData[index];
        auto& desc = m_elementDesc[index];
        desc.SemanticName         = data.semantic;
        desc.SemanticIndex        = data.semanticIndex;
        desc.Format               = GetDxFormat(data);
        desc.InputSlot            = index;
        desc.SemanticIndex        = D3D12_APPEND_ALIGNED_ELEMENT;
        desc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        desc.InstanceDataStepRate = 0;

        size_t byteSize = data.GetSize() * vertexCount;
        auto bufferView = m_vertexBufferView[index];

        bufferView.BufferLocation = m_vertexBuffer->GetAddress() + addrOffset;
        bufferView.SizeInBytes    = byteSize;
        bufferView.StrideInBytes  = data.GetSize();

        addrOffset += byteSize;
    }

    m_indexBufferView.BufferLocation = m_indexBuffer->GetAddress();
    m_indexBufferView.SizeInBytes    = m_indexBuffer->GetByteSize();
    m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;
}

void Dx12Mesh::OnRender(){

    auto cmdList = m_renderResources->GetCommandList();

    cmdList->IASetVertexBuffers(0, m_vertexBufferView.size(), m_vertexBufferView.data());
    cmdList->IASetIndexBuffer(&m_indexBufferView);
    cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

}