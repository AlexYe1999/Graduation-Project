#include "Dx12Model.hpp"
#include "GraphicsManager.hpp"

Dx12Model::Dx12Model(const char* fileName)
    : Model(fileName)
    , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
{

    auto dxDevice = m_graphicsMgr->GetDevice();
    auto cmdList  = m_graphicsMgr->GetTempCommandList();
    uint8_t  frameCount = m_graphicsMgr->GetFrameCount();
    uint32_t numObjectPerFrame = m_model.nodes.size() + 1;
    uint32_t svvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create Resource Heap
    {
        // Create Constant buffer descriptorHeap
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
        cbvHeapDesc.NumDescriptors = frameCount * numObjectPerFrame + m_model.materials.size();
        cbvHeapDesc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(dxDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));

        // Create Perframe Resource
        {

            constexpr uint32_t mainConstByteSize = Utility::CalcAlignment<256>(sizeof(MainConstBuffer));
            constexpr uint32_t objectConstByteSize = Utility::CalcAlignment<256>(sizeof(ObjectConstBuffer));
    
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            for(size_t index = 0; index < frameCount; index++){

                auto& frameResource = m_graphicsMgr->GetFrameResource(index);
                CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(cbvHeap->GetCPUDescriptorHandleForHeapStart());

                // Create Main Constant buffer and view
                frameResource.mainConst = std::make_unique<UploadBuffer>(dxDevice, 1, mainConstByteSize);

                cbvHandle.Offset(index * numObjectPerFrame, svvDescriptorSize);
                cbvDesc.BufferLocation = frameResource.mainConst->GetGpuVirtualAddress();
                cbvDesc.SizeInBytes = mainConstByteSize;

                dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);

                frameResource.objectConst = std::make_unique<UploadBuffer>(dxDevice, numObjectPerFrame, objectConstByteSize);

                // Create Object Constant buffer and view
                size_t virtualAddress = frameResource.objectConst->GetGpuVirtualAddress();
                for(size_t offset = 0; offset < numObjectPerFrame; offset++){

                    cbvHandle.Offset(1, svvDescriptorSize);
                    cbvDesc.BufferLocation = virtualAddress;
                    cbvDesc.SizeInBytes = objectConstByteSize;

                    dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
                    virtualAddress += objectConstByteSize;
                }
            }
        }
    }

    // Create Texture Resource
    {

        auto GetFormat = [&](tinygltf::Image image) -> DXGI_FORMAT{
            switch(image.component){
                case 4:
                {
                    switch(image.bits){
                        case 8:
                        {
                            return DXGI_FORMAT_R8G8B8A8_UNORM;
                        }
                        default:
                            throw std::runtime_error("");
                    }
                    break;
                }
                default:
                    throw std::runtime_error("");
            }
        };

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = -1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle = m_graphicsMgr->GetTexCPUHandle();
        for(auto& tex : m_model.textures){
            auto& image  = m_model.images[tex.source];
            auto  format = GetFormat(image);
            uint64_t srcPitchSize = image.width * 4;
            uint64_t dstPicthSize = Utility::CalcAlignment<D3D12_TEXTURE_DATA_PITCH_ALIGNMENT>(srcPitchSize);

            auto& uploadBuffer = m_uploadBuffers.emplace_back(dxDevice, 1, image.height * dstPicthSize);

            uint64_t srcOffset = 0;
            uint64_t dstOffset = 0;
            for(int row = 0; row < image.height; row++){
                uploadBuffer.CopyData(image.image.data() + srcOffset, srcPitchSize, dstOffset);

                srcOffset += srcPitchSize;
                dstOffset += dstPicthSize;
            }

            auto& texture = textures.emplace_back(dxDevice, cmdList, uploadBuffer, format, image.width, image.height);

            srvDesc.Format = format;
            dxDevice->CreateShaderResourceView(texture.GetResource(), &srvDesc, cpuSrvHandle);
            cpuSrvHandle.Offset(1, svvDescriptorSize);
        }

    }

    // Create Material
    std::vector<uint32_t> texIndex(m_model.materials.size(), 0);
    {
        constexpr uint32_t matConstByteSize = Utility::CalcAlignment<256>(sizeof(MaterialConstant));
        m_uploadBuffers.emplace_back(dxDevice, m_model.materials.size(), matConstByteSize);

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
            numObjectPerFrame * frameCount, svvDescriptorSize
        );

        D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = matConstBuffer->GetGpuVirtualAddress();
        for(uint32_t index = 0; index < m_model.materials.size(); index++){
            auto& mat = m_model.materials[index];
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {virtualAddress, matConstByteSize};
            dxDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);

            D3D12_GPU_DESCRIPTOR_HANDLE texHandle = m_graphicsMgr->GetTexGPUHandle();

            if(mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()){
                auto& pbrParam = mat.extensions["KHR_materials_pbrSpecularGlossiness"];
                if(pbrParam.Has("diffuseTexture")){
                    texIndex[index] = pbrParam.Get("diffuseTexture").Get("index").GetNumberAsInt();
                    texHandle.ptr += svvDescriptorSize * texIndex[index];
                }
            }
            else{

            }
            
            m_materials.emplace_back(new Dx12Material(virtualAddress, texHandle, mat.doubleSided));
            
            virtualAddress += matConstByteSize;
            cbvHandle.Offset(1, svvDescriptorSize);
        }

    }

    // Create Mesh 
    m_meshes.reserve(m_model.meshes.size());
    std::vector<RayTraceMeshInfo> rayTraceMeshInfos;
    std::vector<AccelerationStructerInfo> asInfos;
    uint32_t totalIndexBufferByteSize  = 0;
    uint32_t totalVertexBufferByteSize = 0;
    for(auto& mesh : m_model.meshes){

        StaticMesh* staticMesh = new StaticMesh;
        RayTraceMeshInfo meshInfo;
        AccelerationStructerInfo asInfo;

        for(const auto& primitive : mesh.primitives){
            uint32_t vertexCount = 0;
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
            asInfo.vertexCount = vertexCount;

            auto& accessor = m_model.accessors[primitive.indices];
            assert(
                accessor.type == TINYGLTF_TYPE_SCALAR && 
                accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT
            );
            
            asInfo.indexCount = accessor.count;
            size_t indexBufferByteSize = sizeof(uint32_t) * asInfo.indexCount;
            meshInfo.indexOffsetBytes  = totalIndexBufferByteSize;
            totalIndexBufferByteSize  += indexBufferByteSize;

            m_indexBuffers.emplace_back(dxDevice, indexBufferByteSize, 1);
            m_indexBuffers.back().CopyData(GetBuffer(accessor.bufferView) + accessor.byteOffset, indexBufferByteSize);

            assert(position != nullptr && normal != nullptr);
            switch(primitive.attributes.size()){
                case 2:
                case 3:
                {
                    size_t vertexBufferByteSize = vertex0.GetStructSize() * vertexCount;
                    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(vertexBufferByteSize);

                    meshInfo.positionOffsetBytes = totalVertexBufferByteSize;
                    vertex0.position.CopyToBuffer(data.get(), position, vertexCount);

                    meshInfo.normalOffsetBytes = meshInfo.positionOffsetBytes + 12 * vertexCount;
                    vertex0.normal.CopyToBuffer(data.get(), normal, vertexCount);

                    m_vertexBuffers.emplace_back(dxDevice, vertexBufferByteSize, 1);
                    m_vertexBuffers.back().CopyData(data.get(), vertexBufferByteSize);

                    staticMesh->CreateNewMesh(asInfos.size(), new Dx12Mesh(
                        m_vertexBuffers.back(), vertexCount,
                        m_indexBuffers.back(), accessor.count,
                        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_0,
                        m_materials[primitive.material], dxDevice
                    ));
                    totalVertexBufferByteSize += vertexBufferByteSize;
                    break;
                }
                case 4:
                {
                    assert(tangent != nullptr && texcoord != nullptr);
                    size_t vertexBufferByteSize = vertex1.GetStructSize() * vertexCount;

                    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(vertexBufferByteSize);

                    meshInfo.positionOffsetBytes = totalVertexBufferByteSize;
                    vertex1.position.CopyToBuffer(data.get(), position, vertexCount);
                    
                    meshInfo.normalOffsetBytes = meshInfo.positionOffsetBytes + 12 * vertexCount;
                    vertex1.normal.CopyToBuffer(data.get(), normal, vertexCount);

                    meshInfo.tangentOffsetBytes = meshInfo.normalOffsetBytes + 12 * vertexCount;
                    vertex1.tangent.CopyToBuffer(data.get(), tangent, vertexCount);

                    meshInfo.uvOffsetBytes = meshInfo.tangentOffsetBytes + 16 * vertexCount;
                    vertex1.texCoord.CopyToBuffer(data.get(), texcoord, vertexCount);

                    m_vertexBuffers.emplace_back(dxDevice, vertexBufferByteSize, 1);
                    m_vertexBuffers.back().CopyData(data.get(), vertexBufferByteSize);

                    staticMesh->CreateNewMesh(asInfos.size(), new Dx12Mesh(
                        m_vertexBuffers.back(), vertexCount,
                        m_indexBuffers.back(), accessor.count,
                        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_1,
                        m_materials[primitive.material], dxDevice
                    ));
                    totalVertexBufferByteSize += vertexBufferByteSize;
                    break;
                }
                default:
                {
                    assert(false && "Invalid Vertex Data");
                    break;
                }
            }
            asInfo.texIndex = texIndex[primitive.material];
        }

        m_meshes.emplace_back(staticMesh);
        rayTraceMeshInfos.emplace_back(meshInfo);
        asInfos.emplace_back(asInfo);
    }

    rayTraceIndexBuffer  = std::make_unique<DefaultBuffer>(dxDevice, cmdList, totalIndexBufferByteSize, m_indexBuffers, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    rayTraceVertexBuffer = std::make_unique<DefaultBuffer>(dxDevice, cmdList, totalVertexBufferByteSize, m_vertexBuffers, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    auto& meshInfo = m_uploadBuffers.emplace_back(dxDevice, rayTraceMeshInfos.size(), sizeof(RayTraceMeshInfo));
    meshInfo.CopyData(reinterpret_cast<uint8_t*>(rayTraceMeshInfos.data()), meshInfo.GetByteSize());
    rayTraceMeshInfoGpu = std::make_unique<DefaultBuffer>(dxDevice, cmdList, meshInfo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    // Build Raytracing AS
    {
        uint32_t infoNum = rayTraceMeshInfos.size();
        ComPtr<ID3D12Resource> scrachBuffer;
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(infoNum);
        std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> blasDescs(infoNum);
        std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> bottomLevelPrebuildInfos(infoNum);
        uint64_t neededByteSize = std::numeric_limits<uint64_t>::lowest();
        
        for(size_t i = 0; i < infoNum; i++){

            auto& meshInfo = rayTraceMeshInfos[i];
            auto& asInfo   = asInfos[i];

            D3D12_RAYTRACING_GEOMETRY_DESC& desc = geometryDescs[i];
            desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; 
            
            D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& trianglesDesc = desc.Triangles;
            trianglesDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            trianglesDesc.VertexCount  = asInfo.vertexCount;
            trianglesDesc.VertexBuffer.StartAddress = rayTraceVertexBuffer->GetGpuVirtualAddress() + meshInfo.positionOffsetBytes;
            trianglesDesc.VertexBuffer.StrideInBytes = sizeof(GeoMath::Vector3f);
            trianglesDesc.IndexBuffer = rayTraceIndexBuffer->GetGpuVirtualAddress() + meshInfo.indexOffsetBytes;
            trianglesDesc.IndexCount  = asInfo.indexCount;
            trianglesDesc.IndexFormat = DXGI_FORMAT_R32_UINT;
            trianglesDesc.Transform3x4 = 0;
        
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& blasDesc = blasDescs[i];
            blasDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            blasDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            blasDesc.Inputs.NumDescs = 1;
            blasDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            blasDesc.Inputs.pGeometryDescs = &desc;

            dxDevice->GetRaytracingAccelerationStructurePrebuildInfo(&blasDesc.Inputs, &bottomLevelPrebuildInfos[i]);
            neededByteSize = max(neededByteSize, bottomLevelPrebuildInfos[i].ScratchDataSizeInBytes);
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
        tlasDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        tlasDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        tlasDesc.Inputs.NumDescs = infoNum;
        tlasDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        tlasDesc.Inputs.InstanceDescs = 0;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo;
        dxDevice->GetRaytracingAccelerationStructurePrebuildInfo(&tlasDesc.Inputs, &topLevelPrebuildInfo);
        neededByteSize = max(neededByteSize, topLevelPrebuildInfo.ScratchDataSizeInBytes);

        ThrowIfFailed(dxDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(neededByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(scrachBuffer.GetAddressOf()))
        );

        bottomLevelAccelerationStructures.resize(infoNum);
        for(size_t i = 0; i < infoNum; i++){
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& blasDesc = blasDescs[i];
            ThrowIfFailed(dxDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(
                    bottomLevelPrebuildInfos[i].ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                ), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                nullptr, IID_PPV_ARGS(bottomLevelAccelerationStructures[i].GetAddressOf())
            ));

            blasDesc.DestAccelerationStructureData = bottomLevelAccelerationStructures[i]->GetGPUVirtualAddress();
            blasDesc.ScratchAccelerationStructureData = scrachBuffer->GetGPUVirtualAddress();
            D3D12_RESOURCE_BARRIER uavBarrier = {};
            
            uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarrier.UAV.pResource = bottomLevelAccelerationStructures[i].Get();
            cmdList->ResourceBarrier(1, &uavBarrier);
            cmdList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
        }

        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(infoNum);
        for(size_t i = 0; i < infoNum; i++){
            auto& instanceDesc = instanceDescs[i];
            instanceDesc.Transform[0][0] = 1;
            instanceDesc.Transform[1][1] = 1;
            instanceDesc.Transform[2][2] = 1;

            instanceDesc.InstanceID = i;
            instanceDesc.InstanceMask = 0xFF;
            instanceDesc.InstanceContributionToHitGroupIndex = asInfos[i].texIndex;
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            instanceDesc.AccelerationStructure = bottomLevelAccelerationStructures[i]->GetGPUVirtualAddress();
        }

        for(uint8_t index = 0; index < frameCount; index++){
            auto& frameRes = m_graphicsMgr->GetFrameResource(index);

            ThrowIfFailed(dxDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(
                    topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                ), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                nullptr, IID_PPV_ARGS(frameRes.topLevelAccelerationStructure.GetAddressOf())
            ));

            frameRes.rayTraceInstanceDesc = std::make_unique<UploadBuffer>(
                dxDevice, infoNum, sizeof(D3D12_RAYTRACING_INSTANCE_DESC)
            );

            frameRes.rayTraceInstanceDesc->CopyData(
                reinterpret_cast<uint8_t*>(instanceDescs.data()),
                infoNum*sizeof(D3D12_RAYTRACING_INSTANCE_DESC), 0
            );

            frameRes.scrachData = scrachBuffer;
            frameRes.tlasDesc = tlasDesc;
            frameRes.tlasDesc.Inputs.InstanceDescs = frameRes.rayTraceInstanceDesc->GetGpuVirtualAddress();
            frameRes.tlasDesc.DestAccelerationStructureData = frameRes.topLevelAccelerationStructure->GetGPUVirtualAddress();
            frameRes.tlasDesc.ScratchAccelerationStructureData = scrachBuffer->GetGPUVirtualAddress();
            
            D3D12_RESOURCE_BARRIER uavBarrier = {};
            uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarrier.UAV.pResource = frameRes.topLevelAccelerationStructure.Get();
            cmdList->ResourceBarrier(1, &uavBarrier);
            cmdList->BuildRaytracingAccelerationStructure(&frameRes.tlasDesc, 0, nullptr);

        }
    
    }

    // Create Node
    root = std::make_unique<Scene>();
    for(auto nodeIndex : m_model.scenes[0].nodes){
        root->AddChild(BuildNode(nodeIndex, root.get()));
    }

    m_graphicsMgr->ExecuteCommandList(cmdList);
    m_graphicsMgr->Flush();
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

