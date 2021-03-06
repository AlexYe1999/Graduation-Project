#pragma once
#include "Utility.hpp"
#include "DXSampleHelper.h"
#include "d3dx12.h"
#include <wrl.h>
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <d3dcompiler.h>

namespace Utility{
    
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

        ComPtr<ID3DBlob> blobOut = Utility::D3DReadFileToBlob(csoFileNameInOut);

        if(blobOut == nullptr){
            DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
        
        #if defined (_DEBUG)
            dwShaderFlags |= D3DCOMPILE_DEBUG;
            dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
        #endif

            ComPtr<ID3DBlob> errorBlob = nullptr;
            D3DCompileFromFile(
                Utility::ToWString(hlslFileName).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                entryPoint, shaderModel, dwShaderFlags, 0,
                blobOut.GetAddressOf(), errorBlob.GetAddressOf()
            );

            if(errorBlob != nullptr){
                OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
                assert(0);
            }


            ThrowIfFailed(D3DWriteBlobToFile(blobOut.Get(), Utility::ToWString(csoFileNameInOut).c_str(), FALSE));

        }

        return blobOut;

    }

    namespace Predef{
                                                         
        // Shader  Combination       0x0000 0000 0000 0000 ~ 0x0000 0000 0000 0007 - 3 bits
        // Culling Mode              0x0000 0000 0000 0008 ~ 0x0000 0000 0000 0018 - 2 bits
        // Front   Counter-Clockwise 0x0000 0000 0000 0000 / 0x0000 0000 0000 0010
        
        enum PipelineStateFlag : uint64_t{
            // Mask 00 0111                                0x0007
            PIPELINE_STATE_SHADER_COMB_0             = 0x00000000,
            PIPELINE_STATE_SHADER_COMB_1             = 0x00000001,
            PIPELINE_STATE_SHADER_COMB_2             = 0x00000002,
            PIPELINE_STATE_SHADER_COMB_3             = 0x00000003,
            // Shader State
            // Mask 01 1000                                0x0018
            PIPELINE_STATE_CULL_MODE_NONE            = 0x00000000,
            PIPELINE_STATE_CULL_MODE_FRONT           = 0x00000008,
            PIPELINE_STATE_CULL_MODE_BACK            = 0x00000010,
            // Mask 0000 000 0010 0000                     0x0020
            PIPELINE_STATE_FRONT_CLOCKWISE           = 0x00000000,
            PIPELINE_STATE_FRONT_COUNTER_CLOCKWISE   = 0x00000020,

            // Render Type 
            // Mask 0111 0000 0000 0000                    0x7000
            PIPELINE_STATE_RENDER_BACKBUFFER         = 0x00000000,
            PIPELINE_STATE_RENDER_GBUFFER            = 0x00001000,
            PIPELINE_STATE_RENDER_DXR_RAYTRACING     = 0x00002000,

            // Mask 1000 0000 0000 0000                    0x8000
            PIPELINE_STATE_RENDER_COMPUTE_SHADER           = 0x00008000,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_0        = 0x00000000,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_1        = 0x00000001,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_2        = 0x00000002,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_4        = 0x00000003,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_8        = 0x00000004,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_16       = 0x00000005,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_RP       = 0x00000006,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_READBACK = 0x00000007,
            PIPELINE_STATE_RENDER_COMPUTE_DENOISE_VARIANCE = 0x00000008,

            // Predefine
            PIPELINE_STATE_INITIAL_FLAG              = 0x00000000,
            PIPELINE_STATE_DEFAULT_FLAG              = 0x00000030

        };

    }
}