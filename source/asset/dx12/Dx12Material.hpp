#pragma once
#include "Material.hpp"
#include "GraphicsManager.hpp"

class Dx12Material : public Material{
public:
    Dx12Material(uint32_t matIndex, bool isDoubleSide);

    virtual void OnRender();

protected:
    uint64_t                   m_matFlag;
    Dx12GraphicsManager* const m_graphicsMgr;

};