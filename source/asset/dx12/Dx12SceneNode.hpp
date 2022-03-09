#pragma once
#include "SceneNode.hpp"
#include "Dx12Mesh.hpp"
#include "GraphicsManager.hpp"
#include "Utility.hpp"

class Dx12SceneNode : public SceneNode{
public: 
    Dx12SceneNode(uint32_t nodeIndex, SceneNode* pParentNode) 
        : SceneNode(nodeIndex, pParentNode)
        , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
    {
        FrameCount = m_graphicsMgr->GetFrameCount();
    }

    virtual void OnUpdate() override;
    virtual void OnRender() override;

protected:
    Dx12GraphicsManager* const m_graphicsMgr;
};


class Dx12Camera : public CameraNode{
public:
    Dx12Camera(uint32_t nodeIndex, SceneNode* pParentNode)
        : CameraNode(nodeIndex, pParentNode)
        , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
    {}

    virtual void OnUpdate() override;
    virtual void OnRender() override {};

protected:
    Dx12GraphicsManager* const m_graphicsMgr;
};