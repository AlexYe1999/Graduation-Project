#pragma once
#include "SceneNode.hpp"
#include "Dx12Mesh.hpp"

class Dx12SceneNode : public SceneNode{
public: 
    Dx12SceneNode(uint32_t nodeIndex, SceneNode* pParentNode) 
        : SceneNode(nodeIndex, pParentNode)
    {}

    virtual void OnUpdate() override;
    virtual void OnRender() override {};
protected:

};
