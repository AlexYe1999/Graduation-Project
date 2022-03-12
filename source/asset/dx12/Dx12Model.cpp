#include "Dx12Model.hpp"


Dx12Model::Dx12Model(const char* fileName)
    : Model(fileName)
    , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
{

    auto dxDevice = m_graphicsMgr->GetDevice();
    auto cmdList  = m_graphicsMgr->GetCommandList();
    uint8_t  frameCount = m_graphicsMgr->GetFrameCount();
    uint32_t numObjectPerFrame = m_model.nodes.size() + 1;
    uint32_t cbvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create Resource Heap
    {
        // Create Constant buffer descriptorHeap
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
        cbvHeapDesc.NumDescriptors = frameCount * numObjectPerFrame + m_model.materials.size() + m_model.materials.size();
        cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(dxDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));

        // Create Perframe Resource
        {

            constexpr size_t mainConstByteSize = Utility::CalcAlignment<256>(sizeof(MainConstBuffer));
            constexpr size_t objectConstByteSize = Utility::CalcAlignment<256>(sizeof(ObjectConstBuffer));
    
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            for(size_t index = 0; index < frameCount; index++){

                auto& frameResource = m_graphicsMgr->GetFrameResource(index);
                CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(cbvHeap->GetCPUDescriptorHandleForHeapStart());

                // Create Main Constant buffer and view
                frameResource.mainConst = std::make_unique<UploadBuffer>(dxDevice, mainConstByteSize);

                cbvHandle.Offset(index * numObjectPerFrame, cbvDescriptorSize);
                cbvDesc.BufferLocation = frameResource.mainConst->GetAddress();
                cbvDesc.SizeInBytes = mainConstByteSize;

                dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);

                frameResource.objectConst = std::make_unique<UploadBuffer>(dxDevice, objectConstByteSize * numObjectPerFrame);

                // Create Object Constant buffer and view
                size_t virtualAddress = frameResource.objectConst->GetAddress();
                for(size_t offset = 0; offset < numObjectPerFrame; offset++){

                    cbvHandle.Offset(1, cbvDescriptorSize);
                    cbvDesc.BufferLocation = virtualAddress;
                    cbvDesc.SizeInBytes = objectConstByteSize;

                    dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
                    virtualAddress += objectConstByteSize;
                }

            }

        }

    }

    // Create Material
    {
        constexpr uint32_t matConstByteSize = Utility::CalcAlignment<256>(sizeof(MaterialConstant));
        m_uploadBuffers.emplace_back(dxDevice, m_model.materials.size() * matConstByteSize);

        MaterialConstant matConst;
        for(size_t index = 0; index < m_model.materials.size(); index++){
            auto& mat = m_model.materials[index];
        
            matConst.alphaCutoff = static_cast<float>(mat.alphaCutoff);
            matConst.essisiveFactor = GeoMath::Vector3f(
                static_cast<float>(mat.emissiveFactor[0]),
                static_cast<float>(mat.emissiveFactor[1]),
                static_cast<float>(mat.emissiveFactor[2])
            );

            if(mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()){
                auto& pbrParam   = mat.extensions["KHR_materials_pbrSpecularGlossiness"];
                auto& diffuse    = pbrParam.Get("diffuseFactor");
                auto& specular   = pbrParam.Get("specularFactor");
                auto& glossiness = pbrParam.Get("glossinessFactor");
            
                matConst.diffuseFactor = GeoMath::Vector4f(
                    static_cast<float>(diffuse.Get(0).GetNumberAsDouble()),
                    static_cast<float>(diffuse.Get(1).GetNumberAsDouble()),
                    static_cast<float>(diffuse.Get(2).GetNumberAsDouble()),
                    static_cast<float>(diffuse.Get(3).GetNumberAsDouble())
                );

                matConst.specularFactor = GeoMath::Vector3f(
                    static_cast<float>(specular.Get(0).GetNumberAsDouble()),
                    static_cast<float>(specular.Get(1).GetNumberAsDouble()),
                    static_cast<float>(specular.Get(2).GetNumberAsDouble())
                );

                matConst.glossinessFactor = static_cast<float>(glossiness.GetNumberAsDouble());
            }
            else{
                auto& pbrParam = mat.pbrMetallicRoughness;

            }

            m_uploadBuffers.back().CopyData(
                reinterpret_cast<uint8_t*>(&matConst),
                matConstByteSize, index * matConstByteSize
            );

        }

        matConstBuffer = std::make_unique<DefaultBuffer>(dxDevice, cmdList, m_uploadBuffers.back());
        m_materials.reserve(m_model.materials.size());

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(
            cbvHeap->GetCPUDescriptorHandleForHeapStart(),
            numObjectPerFrame * frameCount, cbvDescriptorSize
        );

        D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = matConstBuffer->GetAddress();
        for(uint32_t index = 0; index < m_model.materials.size(); index++){
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {virtualAddress, matConstByteSize};
            dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
            m_materials.emplace_back(new Dx12Material(virtualAddress, m_model.materials[index].doubleSided));
            
            virtualAddress += matConstByteSize;
            cbvHandle.Offset(1, cbvDescriptorSize);
        }

    }

    // Create Mesh 
    m_meshes.reserve(m_model.meshes.size());
    uint64_t virtualAddress = matConstBuffer->GetAddress();
    for(auto& mesh : m_model.meshes){

        StaticMesh* staticMesh = new StaticMesh;
        for(const auto& primitive : mesh.primitives){
            size_t vertexCount = 0;
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
            m_indexBuffers.emplace_back(dxDevice, indexBufferSize);
            m_indexBuffers.back().CopyData(GetBuffer(accessor.bufferView) + accessor.byteOffset, indexBufferSize);

            assert(position != nullptr && normal != nullptr);
            switch(primitive.attributes.size()){
                case 2:
                case 3:
                {
                    size_t vertexBufferSize = vertex0.GetStructSize() * vertexCount;
                    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(vertexBufferSize);

                    vertex0.position.CopyToBuffer(data.get(), position, vertexCount);
                    vertex0.normal.CopyToBuffer(data.get(), normal, vertexCount);

                    m_vertexBuffers.emplace_back(dxDevice, vertexBufferSize);
                    m_vertexBuffers.back().CopyData(data.get(), vertexBufferSize);

                    staticMesh->CreateNewMesh(new Dx12Mesh(
                        m_vertexBuffers.back(), vertexCount,
                        m_indexBuffers.back(), accessor.count,
                        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_0,
                        m_materials[primitive.material], dxDevice
                    ));
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

                    m_vertexBuffers.emplace_back(dxDevice, vertexBufferSize);
                    m_vertexBuffers.back().CopyData(data.get(), vertexBufferSize);

                    staticMesh->CreateNewMesh(new Dx12Mesh(
                        m_vertexBuffers.back(), vertexCount,
                        m_indexBuffers.back(), accessor.count,
                        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_1,
                        m_materials[primitive.material], dxDevice
                    ));
                    break;
                }
                default:
                    assert(false && "Invalid Vertex Data");
                    break;
            }
        }

        m_meshes.emplace_back(staticMesh);
    }

    // Create All Resource
    root = std::make_unique<Scene>();
    for(auto nodeIndex : m_model.scenes[0].nodes){
        root->AddChild(BuildNode(nodeIndex, root.get()));
    }

}

std::unique_ptr<SceneNode> Dx12Model::BuildNode(size_t nodeIndex, SceneNode* pParentNode){

    auto glNode = m_model.nodes[nodeIndex];
    std::unique_ptr<SceneNode> sceneNode(new Dx12SceneNode(nodeIndex, pParentNode));

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
            matrix[0],  matrix[1],  matrix[2],  matrix[3],
            matrix[4],  matrix[5],  matrix[6],  matrix[7],
            matrix[8],  matrix[9],  matrix[10], matrix[11],
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
        sceneNode->AddChild(BuildNode(childIndex, sceneNode.get()));
    }

    return sceneNode;
}

