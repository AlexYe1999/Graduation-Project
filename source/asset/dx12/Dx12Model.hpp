#pragma once
#include "Dx12SceneNode.hpp"
#include "Dx12Material.hpp"
#include "DxUtility.hpp"
#include "Model.hpp"
#include "GraphicsManager.hpp"

struct Dx12Model final : public Model{
public:
    Dx12Model(const char* fileName);
    
    //std::unique_ptr<Scene> LoadToDx12Scene(const ComPtr<ID3D12Device8>&, const ComPtr<ID3D12GraphicsCommandList2>&);

    std::unique_ptr<Scene>                    root;
    std::unique_ptr<DefaultBuffer>            matConstBuffer;

    ComPtr<ID3D12DescriptorHeap>              cbvHeap;
    //std::vector<std::shared_ptr<Dx12Texture>> m_texturess;

private:
    Dx12GraphicsManager*                      m_graphicsMgr;
    std::vector<UploadBuffer>                 m_vertexBuffers;
    std::vector<UploadBuffer>                 m_indexBuffers;
    std::vector<UploadBuffer>                 m_uploadBuffers;

    std::vector<std::shared_ptr<IComponent>>  m_meshes;
    std::vector<std::shared_ptr<Material>>    m_materials;

    std::unique_ptr<SceneNode> BuildNode(size_t nodeIndex, SceneNode* pParentNode);


};

