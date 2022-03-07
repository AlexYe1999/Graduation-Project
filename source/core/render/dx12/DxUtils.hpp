#pragma once
#include "Utils.hpp"
#include "DXSampleHelper.h"
#include "d3dx12.h"
#include <d3d11.h>
#include <d3d12.h>
#include <d3dcompiler.h>

namespace DxUtils{
    
    inline ComPtr<ID3DBlob> D3DReadFileToBlob(const char* fileName){

        ComPtr<ID3DBlob> blob;
        std::ifstream fin(fileName, std::ios::binary);

        if(fin.fail()) return blob;

        fin.seekg(0, std::ios::end);
        std::streamsize size = static_cast<std::streamsize>(fin.tellg());
        fin.seekg(0, std::ios::beg);

        D3DCreateBlob(size, blob.GetAddressOf());

        fin.read(static_cast<char*>(blob->GetBufferPointer()), size);
        fin.close();

        return blob;
    }

    inline ComPtr<ID3DBlob> CreateShaderFromFile(
        const char* csoFileNameInOut, const char* hlslFileName, 
        const char* entryPoint, const char* shaderModel
    ){   

        ComPtr<ID3DBlob> blobOut = DxUtils::D3DReadFileToBlob(csoFileNameInOut);

        if(blobOut == nullptr){
            DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
        
        #if defined (_DEBUG)
            dwShaderFlags |= D3DCOMPILE_DEBUG;
            dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
        #endif

            ComPtr<ID3DBlob> errorBlob = nullptr;
            ThrowIfFailed(D3DCompileFromFile(
                Utils::ToWString(hlslFileName).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                entryPoint, shaderModel, dwShaderFlags, 0,
                blobOut.GetAddressOf(), errorBlob.GetAddressOf()
            ));

            if(errorBlob != nullptr){
                OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
                assert(0);
            }


            ThrowIfFailed(D3DWriteBlobToFile(blobOut.Get(), Utils::ToWString(csoFileNameInOut).c_str(), FALSE));

        }

        return blobOut;

    }

}