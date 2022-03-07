#pragma once
#include "Dx12SceneNode.hpp"
#include "DxUtils.hpp"
#include "Model.hpp"
#include "RenderResource.hpp"

class Dx12Model : public Model{
public:
    Dx12Model(const char* fileName, RenderResource* const renderResource);
    
    //std::unique_ptr<Scene> LoadToDx12Scene(const ComPtr<ID3D12Device8>&, const ComPtr<ID3D12GraphicsCommandList2>&);

    std::unique_ptr<Scene>                    root;
    //std::vector<std::shared_ptr<Dx12Texture>> m_texturess;

private:

    ComPtr<ID3D12Device8>                     m_device;

    std::vector<UploadBuffer>                 m_vertexBuffer;
    std::vector<UploadBuffer>                 m_indexBuffer;
    std::vector<UploadBuffer>                 m_uploadBuffer;

    std::vector<std::shared_ptr<IComponent>>  m_meshes;

    std::unique_ptr<SceneNode> BuildNode(size_t nodeIndex, SceneNode* pParentNode, RenderResource* const renderResource);
};

