#include "Dx12Model.hpp"


Dx12Model::Dx12Model(
    const char* fileName, RenderResource* const renderResource
)
    : Model(fileName)
{

    static Vertex0 vertex0;
    static Vertex1 vertex1;

    auto dxDevice = renderResource->GetDevice();

    // Create Constant Buffer and Texture Resource 
    renderResource->CreateRenderResource(m_model.nodes.size(), 0, 0);
    // Create PipelineStateObject
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.pRootSignature = renderResource->rootSignatureDeferred.Get();
        psoDesc.InputLayout = {vertex0.inputLayout.data(), static_cast<unsigned int>(vertex0.inputLayout.size())};
        psoDesc.VS = renderResource->vertexShaders[0]->GetByteCode();
        psoDesc.PS = renderResource->pixelShaders[0]->GetByteCode();
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.FrontCounterClockwise = true;
        psoDesc.RasterizerState.MultisampleEnable = false;
        psoDesc.RasterizerState.DepthClipEnable = true;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat     = DXGI_FORMAT_D24_UNORM_S8_UINT;

        ThrowIfFailed(dxDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(renderResource->pipelineStateObjects[0].GetAddressOf())));
        
        psoDesc.VS = renderResource->vertexShaders[1]->GetByteCode();
        psoDesc.PS = renderResource->pixelShaders[1]->GetByteCode();
        psoDesc.InputLayout = {vertex1.inputLayout.data(), static_cast<unsigned int>(vertex1.inputLayout.size())};

        ThrowIfFailed(dxDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(renderResource->pipelineStateObjects[1].GetAddressOf())));
    }


    // Create Mesh 
    m_meshes.reserve(m_model.meshes.size());
    for(auto& mesh : m_model.meshes){

        StaticMesh* staticMesh = new StaticMesh;
        for(const auto& primitive : mesh.primitives){
            size_t   vertexCount = 0;
            uint8_t* position = nullptr;
            uint8_t* normal   = nullptr;
            uint8_t* tangent  = nullptr;
            uint8_t* texcoord = nullptr;
            
            assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);

            for(const auto& attributes : primitive.attributes){
                const std::string attrName = attributes.first;
                auto& accessor = m_model.accessors[attributes.second];

                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                if(attrName == "POSITION"){
                    assert(accessor.type == TINYGLTF_TYPE_VEC3);

                    position = GetBuffer(accessor.bufferView) + accessor.byteOffset;
                }
                else if(attrName == "NORMAL"){
                    assert(accessor.type == TINYGLTF_TYPE_VEC3);

                    normal = GetBuffer(accessor.bufferView) + accessor.byteOffset;
                }
                else if(attrName == "TANGENT"){
                    assert(accessor.type == TINYGLTF_TYPE_VEC4);
                    tangent = GetBuffer(accessor.bufferView) + accessor.byteOffset;
                }
                else if(attrName == "TEXCOORD_0"){
                    assert(accessor.type == TINYGLTF_TYPE_VEC2);
                    texcoord = GetBuffer(accessor.bufferView) + accessor.byteOffset;
                }
                vertexCount = accessor.count;

            }

            auto& accessor = m_model.accessors[primitive.indices];
            assert(
                accessor.type == TINYGLTF_TYPE_SCALAR && 
                accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT
            );
            
            size_t indexBufferSize = sizeof(uint32_t) * accessor.count;
            m_indexBuffer.emplace_back(dxDevice, indexBufferSize);
            m_indexBuffer.back().CopyData(GetBuffer(accessor.bufferView) + accessor.byteOffset, indexBufferSize);

            assert(position != nullptr && normal != nullptr);
            switch(primitive.attributes.size()){
                case 2:
                case 3:
                {
                    size_t vertexBufferSize = vertex0.GetStructSize() * vertexCount;
                    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(vertexBufferSize);

                    vertex0.position.CopyToBuffer(data.get(), position, vertexCount);
                    vertex0.normal.CopyToBuffer(data.get(), normal, vertexCount);

                    m_vertexBuffer.emplace_back(dxDevice, vertexBufferSize);
                    m_vertexBuffer.back().CopyData(data.get(), vertexBufferSize);

                    staticMesh->CreateNewMesh(new Dx12Mesh(
                        m_vertexBuffer.back(), vertexCount,
                        m_indexBuffer.back(), accessor.count,
                        vertex0, dxDevice, renderResource)
                    );
                    break;
                }
                case 4:
                {

                    assert(tangent != nullptr && texcoord != nullptr);
                    size_t vertexBufferSize = vertex1.GetStructSize() * vertexCount;
                    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(vertexBufferSize);

                    vertex1.position.CopyToBuffer(data.get(), position, vertexCount);
                    vertex1.normal.CopyToBuffer(data.get(), normal, vertexCount);
                    vertex1.tangent.CopyToBuffer(data.get(), tangent, vertexCount);
                    vertex1.texCoord.CopyToBuffer(data.get(), texcoord, vertexCount);

                    m_vertexBuffer.emplace_back(dxDevice, vertexBufferSize);
                    m_vertexBuffer.back().CopyData(data.get(), vertexBufferSize);

                    staticMesh->CreateNewMesh(new Dx12Mesh(
                        m_vertexBuffer.back(), vertexCount,
                        m_indexBuffer.back(), accessor.count,
                        vertex1, dxDevice, renderResource)
                    );
                    break;
                }
                default:
                    assert(false && "Invalid Vertex Data");
                    break;
            }
        }

        m_meshes.emplace_back(staticMesh);
    }

    root = std::make_unique<Scene>();
    for(auto nodeIndex : m_model.scenes[0].nodes){
        root->AddChild(BuildNode(nodeIndex, root.get(), renderResource));
    }

}

std::unique_ptr<SceneNode> Dx12Model::BuildNode(
    size_t nodeIndex, SceneNode* pParentNode, RenderResource* const renderResource
){

    auto glNode = m_model.nodes[nodeIndex];
    std::unique_ptr<SceneNode> sceneNode(new Dx12SceneNode(nodeIndex, pParentNode, renderResource));

    GeoMath::Matrix4f S, R, T;

    auto& scale = glNode.scale;
    if(scale.size() > 0){
        S = GeoMath::Matrix4f::Scale(scale[0], scale[1], scale[2]);
    }

    auto& rotation = glNode.rotation;
    if(rotation.size() > 0){
        R = GeoMath::Matrix4f::Rotation(rotation[0], rotation[1], rotation[2], rotation[3]);
    }

    auto& translation = glNode.translation;
    if(translation.size() > 0){
        T = GeoMath::Matrix4f::Translation(translation[0], translation[1], translation[2]);
    }
   
    sceneNode->SetMatrix(&S, &R, &T);

    auto& matrix = glNode.matrix;

#define INVERSE
    if(matrix.size() > 0){

        GeoMath::Matrix4f toWorld(
            matrix[0], matrix[1], matrix[2], matrix[3],
            matrix[4], matrix[5], matrix[6], matrix[7],
            matrix[8], matrix[9], matrix[10], matrix[11],
            matrix[12], matrix[13], matrix[14], matrix[15]
        );

        sceneNode->SetMatrix(toWorld);
    }

    // Add Mesh to SceneNode
    if(glNode.mesh >= 0){
        sceneNode->AddComponent(m_meshes[glNode.mesh]);
    }

    // Add ChildNode to ParentNode
    for(auto childIndex : glNode.children){
        sceneNode->AddChild(BuildNode(childIndex, sceneNode.get(), renderResource));
    }

    return sceneNode;
}

