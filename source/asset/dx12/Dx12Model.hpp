#pragma once
#include "Dx12SceneNode.hpp"
#include "Dx12Material.hpp"
#include "Texture2D.hpp"
#include "DxUtility.hpp"
#include "Model.hpp"

struct AccelerationStructerInfo{
    uint32_t vertexCount = 0;
    uint32_t indexCount  = 0;
    uint32_t texIndex    = 0;
};

struct Dx12Model final : public Model{
public:
    Dx12Model(const char* fileName);
    
    std::unique_ptr<Scene>                    root;
    std::unique_ptr<DefaultBuffer>            matConstBuffer;

    ComPtr<ID3D12DescriptorHeap>              cbvHeap;
    std::vector<Texture2D>                    textures;

    // RayTracing Resource
    std::unique_ptr<DefaultBuffer>            rayTraceMeshInfoGpu;
    std::unique_ptr<DefaultBuffer>            rayTraceIndexBuffer;
    std::unique_ptr<DefaultBuffer>            rayTraceVertexBuffer;

    std::vector<ComPtr<ID3D12Resource>>       bottomLevelAccelerationStructures;

private:
    Dx12GraphicsManager*                      m_graphicsMgr;

    std::vector<UploadBuffer>                 m_vertexBuffers;
    std::vector<UploadBuffer>                 m_indexBuffers;
    std::vector<UploadBuffer>                 m_uploadBuffers;

    std::vector<std::shared_ptr<IComponent>>  m_meshes;
    std::vector<std::shared_ptr<Material>>    m_materials;

    std::unique_ptr<SceneNode> BuildNode(size_t nodeIndex, SceneNode* pParentNode);

};

