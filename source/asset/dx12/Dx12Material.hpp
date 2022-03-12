#pragma once
#include "Material.hpp"
#include "GraphicsManager.hpp"

class Dx12Material : public Material{
public:
    Dx12Material(D3D12_GPU_VIRTUAL_ADDRESS matAddress, bool isDoubleSide);

    virtual void OnRender();

protected:
    uint64_t                   m_matFlag;
    Dx12GraphicsManager* const m_graphicsMgr;
    D3D12_GPU_VIRTUAL_ADDRESS  m_matAddress;

};