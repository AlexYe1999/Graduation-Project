#pragma once
#include "tiny_gltf.h"
#include "Texture.hpp"
#include "DxUtility.hpp"
#include "Texture2D.hpp"

class Dx12Texture : public Texture{
public:
    const IDXGIResource* GetResource() const { m_tex2D->GetResource(); }

protected:
    std::unique_ptr<Texture2D> m_tex2D;
};