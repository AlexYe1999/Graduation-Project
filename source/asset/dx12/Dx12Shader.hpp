#pragma once
#include "Shader.hpp"
#include "DxUtility.hpp"

struct Dx12Shader : public Shader{
public:
    Dx12Shader(const char* fileName, const char* shaderType)
        : Shader(fileName)
    {
        m_blob = Utility::CreateShaderFromFile(
            std::string("assets\\shader\\" + std::string(fileName) + ".cso").c_str(),
            std::string("assets\\shader\\" + std::string(fileName) + ".hlsl").c_str(),
            "main", shaderType
        );
    }

    D3D12_SHADER_BYTECODE GetByteCode() const{ return { m_blob->GetBufferPointer(), m_blob->GetBufferSize() };}

protected:

    ComPtr<ID3DBlob> m_blob;
};
