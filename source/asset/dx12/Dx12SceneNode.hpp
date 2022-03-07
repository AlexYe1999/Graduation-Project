#pragma once
#include "SceneNode.hpp"
#include "Dx12Mesh.hpp"
#include "RenderResource.hpp"
#include "Utils.hpp"

class Dx12SceneNode : public SceneNode{
public: 
    Dx12SceneNode(uint32_t nodeIndex, SceneNode* pParentNode, RenderResource* const renderResource) 
        : SceneNode(nodeIndex, pParentNode)
        , m_renderResource(renderResource)
    {}

    virtual void OnUpdate() override;
    virtual void OnRender() override;

protected:
    RenderResource* const m_renderResource;
};


class Dx12Camera : public CameraNode{
public:
    Dx12Camera(uint32_t nodeIndex, SceneNode* pParentNode, RenderResource* const renderResource)
        : CameraNode(nodeIndex, pParentNode)
        , m_renderResource(renderResource)
    {}

    virtual void OnUpdate() override;
    virtual void OnRender() override {};

protected:
    RenderResource* const m_renderResource;
};