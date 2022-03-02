#include "Dx12Model.hpp"


Dx12Model::Dx12Model(
    const char* fileName, RenderResource* const renderResource
)
    : Model(fileName)
{

    auto dxDevice = renderResource->GetDevice();

    root = std::make_unique<Scene>();
   
    m_meshes.reserve(m_model.meshes.size());
    for(auto& mesh : m_model.meshes){

        StaticMesh* staticMesh(new StaticMesh);
        for(const auto& primitive : mesh.primitives){
            size_t   vertexCount = 0;
            uint8_t* position = nullptr;
            uint8_t* normal   = nullptr;
            uint8_t* tangent  = nullptr;
            uint8_t* texcoord = nullptr;

            for(const auto& attributes : primitive.attributes){
                auto& accessor = m_model.accessors[attributes.second];
                const std::string attrName = attributes.first;

                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                if(attrName == "POSITION"){
                    assert(accessor.type == TINYGLTF_TYPE_VEC3);

                    position = GetBuffer(accessor.bufferView);
                }
                else if(attrName == "NORMAL"){
                    assert(accessor.type == TINYGLTF_TYPE_VEC3);

                    normal = GetBuffer(accessor.bufferView);
                }
                else if(attrName == "TANGENT"){
                    assert(accessor.type == TINYGLTF_TYPE_VEC4);
                    tangent = GetBuffer(accessor.bufferView);
                }
                else if(attrName == "TEXCOORD_0"){
                    assert(accessor.type == TINYGLTF_TYPE_VEC2);
                    texcoord = GetBuffer(accessor.bufferView);
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
            m_indexBuffer.back().CopyData(GetBuffer(accessor.bufferView), indexBufferSize);

            assert(position != nullptr && normal != nullptr);
            switch(primitive.attributes.size()){
                case 2:
                case 3:
                {
                    static Vertex0 vertex0;
                    size_t vertexBufferSize = vertex0.GetStructSize() * vertexCount;
                    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(vertexBufferSize);

                    vertex0.position.CopyToBuffer(data.get(), position, vertexCount);
                    vertex0.normal.CopyToBuffer(data.get(), normal, vertexCount);

                    m_vertexBuffer.emplace_back(dxDevice, vertexBufferSize);
                    m_vertexBuffer.back().CopyData(data.get(), vertexBufferSize);

                    staticMesh->CreateNewMesh(new Dx12Mesh(
                        m_vertexBuffer.back(), vertexCount,
                        m_indexBuffer.back(), accessor.count,
                        vertex0.GetVarTypeData(), dxDevice, renderResource)
                    );
                    break;
                }
                case 4:
                {
                    assert(tangent != nullptr && texcoord != nullptr);
                    static Vertex1 vertex1;
                    size_t vertexBufferSize = vertex1.GetStructSize() * vertexCount;
                    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(vertexBufferSize);

                    vertex1.position.CopyToBuffer(data.get(), position, vertexCount);
                    vertex1.normal.CopyToBuffer(data.get(), normal, vertexCount);
                    vertex1.tangent.CopyToBuffer(data.get(), tangent, vertexCount);
                    vertex1.texcoord.CopyToBuffer(data.get(), texcoord, vertexCount);

                    m_vertexBuffer.emplace_back(dxDevice, vertexBufferSize);
                    m_vertexBuffer.back().CopyData(data.get(), vertexBufferSize);

                    staticMesh->CreateNewMesh(new Dx12Mesh(
                        m_vertexBuffer.back(), vertexCount,
                        m_indexBuffer.back(), accessor.count,
                        vertex1.GetVarTypeData(), dxDevice, renderResource)
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

    root = std::unique_ptr<Scene>();
    for(auto nodeIndex : m_model.scenes[0].nodes){
        BuildNode(root.get(), 0);
    }

}


void Dx12Model::BuildNode(SceneNode* parentNode, size_t nodeIndex){



}

