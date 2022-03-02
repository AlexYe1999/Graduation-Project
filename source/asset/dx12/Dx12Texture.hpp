#pragma once
#include "Texture.hpp"
#include "DxUtils.hpp"

class Dx12Texture : public Texture{
public:
    Dx12Texture();

    static void SetCurrentCommandList(const ComPtr<ID3D12GraphicsCommandList2>& device){
        m_currentCommandList = device;
    }

protected:
    inline static ComPtr<ID3D12GraphicsCommandList2> m_currentCommandList = nullptr;
};