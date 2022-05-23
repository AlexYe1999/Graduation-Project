#include "AppFramework.hpp"
#include "Dx12Model.hpp"
#include "Pipeline.hpp"
#include "d3dx12.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

#include "dxcapi.h"
#include "dxcapi.use.h"

Pipeline::Pipeline(std::string& name, uint16_t width, uint16_t height)
    : Application(name, width, height)
    , m_openDenoising(false)
    , m_openFrameBlend(false)
    , m_openReprojection(false)
    , m_alpha(1.0f)
    , m_beta(1.0f)
    , m_gamma(1.0f)
    , m_sigma(1.0f)
    , m_isLeftMouseDown(false)
    , m_lastMousePosX(0)
    , m_lastMousePosY(0)
    , m_cameraMode(0)
    , m_camera(nullptr)
    , m_rtvDescriptorSize(0)
    , m_dsvDescriptorSize(0)
    , m_svvDescriptorSize(0)
    , m_texConstViewOffset(0)
    , m_gbufferViewOffset(0)
    , m_backgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
    , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
    , m_viewport{}
    , m_scissors{}
    , m_dispatchRayDesc{}
{}

Pipeline::~Pipeline(){}

void Pipeline::OnInit(){
    InitD3D();
    InitGUI();
}

void Pipeline::InitD3D(){

    m_graphicsMgr->OnInit(AppFramework::GetWnd(), 2, m_wndWidth, m_wndHeight);

    auto& dxDevice = m_graphicsMgr->GetDevice();
    auto  cmdList  = m_graphicsMgr->GetTempCommandList();

    {
        const D3D12_STATIC_SAMPLER_DESC samplers[6] = {
            CD3DX12_STATIC_SAMPLER_DESC(
                0,                                // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP   // addressW
            ),
            CD3DX12_STATIC_SAMPLER_DESC(
                1,                                // shaderRegister
                D3D12_FILTER_ANISOTROPIC,         // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
                0.0f,                             // mipLODBias
                8                                 // maxAnisotropy
            ),
            CD3DX12_STATIC_SAMPLER_DESC(
                2,                                // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP   // addressW
            ),
            CD3DX12_STATIC_SAMPLER_DESC(
                3,                                // shaderRegister
                D3D12_FILTER_ANISOTROPIC,         // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
                0.0f,                             // mipLODBias
                8                                 // maxAnisotropy
            ),
            CD3DX12_STATIC_SAMPLER_DESC(
                4,                                // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP   // addressW
            ),
            CD3DX12_STATIC_SAMPLER_DESC(
                5,                                // shaderRegister
                D3D12_FILTER_ANISOTROPIC,         // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
                0.0f,                             // mipLODBias
                8                                 // maxAnisotropy
            )
        };

        ComPtr<ID3DBlob> serializedRootSignature = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        {
            // Create Deffered Rendering Rootsignature
            D3D12_DESCRIPTOR_RANGE gbuffer{
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                5, 0, 0, 0
            };
            D3D12_DESCRIPTOR_RANGE luminance{
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                1, 5, 0, 0
            };
            CD3DX12_ROOT_PARAMETER rootParameter[5] = {};
            rootParameter[0].InitAsConstantBufferView(1);
            rootParameter[1].InitAsConstantBufferView(0);
            rootParameter[2].InitAsConstantBufferView(2);
            rootParameter[3].InitAsDescriptorTable(1, &gbuffer);
            rootParameter[4].InitAsDescriptorTable(1, &luminance);

            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
                5, rootParameter, 2, samplers,
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            );

            ThrowIfFailed(D3D12SerializeRootSignature(
                &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf()
            ));

            ThrowIfFailed(dxDevice->CreateRootSignature(
                0, serializedRootSignature->GetBufferPointer(),
                serializedRootSignature->GetBufferSize(),
                IID_PPV_ARGS(m_deferredRootSignature.GetAddressOf())
            ));
        }

        {
             // RayTracing 
            D3D12_DESCRIPTOR_RANGE rayTracingRange{
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                1, 0, 1, 0
            };
            // Raytracing Rootsignature
            CD3DX12_ROOT_PARAMETER globalRayteacingRootParameter[7] = {};
            globalRayteacingRootParameter[0].InitAsDescriptorTable(1, &rayTracingRange);
            globalRayteacingRootParameter[1].InitAsConstantBufferView(0, 1);
            globalRayteacingRootParameter[2].InitAsShaderResourceView(0, 1);
            globalRayteacingRootParameter[3].InitAsShaderResourceView(1, 1);
            globalRayteacingRootParameter[4].InitAsShaderResourceView(2, 1);
            globalRayteacingRootParameter[5].InitAsShaderResourceView(3, 1);
            globalRayteacingRootParameter[6].InitAsShaderResourceView(4, 1);

            CD3DX12_ROOT_SIGNATURE_DESC golobalRootSignatureDesc(
                7, globalRayteacingRootParameter, 2, &samplers[2],
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            );

            ThrowIfFailed(D3D12SerializeRootSignature(
                &golobalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf()
            ));

            ThrowIfFailed(dxDevice->CreateRootSignature(
                0, serializedRootSignature->GetBufferPointer(),
                serializedRootSignature->GetBufferSize(),
                IID_PPV_ARGS(m_rayTracingGlobalRootSignature.GetAddressOf())
            ));

            D3D12_DESCRIPTOR_RANGE rayTracingCLocalRange{
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                1, 0, 2, 0
            };
            CD3DX12_ROOT_PARAMETER hitLocalRayteacingRootParameter[1] = {};
            hitLocalRayteacingRootParameter[0].InitAsDescriptorTable(1, &rayTracingCLocalRange);

            CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(
                1, hitLocalRayteacingRootParameter, 0, nullptr,
                D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
            );

            ThrowIfFailed(D3D12SerializeRootSignature(
                &localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf()
            ));

            ThrowIfFailed(dxDevice->CreateRootSignature(
                0, serializedRootSignature->GetBufferPointer(),
                serializedRootSignature->GetBufferSize(),
                IID_PPV_ARGS(m_hitLocalRootSignature.GetAddressOf())
            ));

            D3D12_DESCRIPTOR_RANGE finalResultTable{
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                1, 0, 3, 0
            };
            D3D12_DESCRIPTOR_RANGE varianceTable{
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                1, 1, 3, 0
            };
            D3D12_DESCRIPTOR_RANGE preResultTable{
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                1, 0, 3, 0
            };
            D3D12_DESCRIPTOR_RANGE newResultTable{
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                1, 1, 3, 0
            };
            D3D12_DESCRIPTOR_RANGE gBufferTable{
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                5, 2, 3, 0
            };

            CD3DX12_ROOT_PARAMETER denisingRootParameter[6]{};
            denisingRootParameter[0].InitAsConstantBufferView(0);
            denisingRootParameter[1].InitAsDescriptorTable(1, &finalResultTable);
            denisingRootParameter[2].InitAsDescriptorTable(1, &varianceTable);
            denisingRootParameter[3].InitAsDescriptorTable(1, &preResultTable);
            denisingRootParameter[4].InitAsDescriptorTable(1, &newResultTable);
            denisingRootParameter[5].InitAsDescriptorTable(1, &gBufferTable);

            CD3DX12_ROOT_SIGNATURE_DESC denisingSignatureDesc(
                6, denisingRootParameter, 2, &samplers[4],
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            );

            ThrowIfFailed(D3D12SerializeRootSignature(
                &denisingSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf()
            ));

            ThrowIfFailed(dxDevice->CreateRootSignature(
                0, serializedRootSignature->GetBufferPointer(),
                serializedRootSignature->GetBufferSize(),
                IID_PPV_ARGS(m_denoisingRootSignature.GetAddressOf())
            ));

        }

    }

    uint64_t layoutFlags[2] = {
        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_0,
        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_1
    };

    uint64_t matFlags[2] = {
        PipelineStateFlag::PIPELINE_STATE_CULL_MODE_NONE | PipelineStateFlag::PIPELINE_STATE_FRONT_COUNTER_CLOCKWISE,
        PipelineStateFlag::PIPELINE_STATE_CULL_MODE_BACK | PipelineStateFlag::PIPELINE_STATE_FRONT_COUNTER_CLOCKWISE
    };

    for(auto layoutFlag : layoutFlags){
        for(auto matFlag : matFlags){
            m_graphicsMgr->CreatePipelineStateObject(
                PipelineStateFlag::PIPELINE_STATE_RENDER_GBUFFER | layoutFlag | matFlag,
                m_deferredRootSignature
            );
        }
    }

    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_2  |
        PipelineStateFlag::PIPELINE_STATE_CULL_MODE_BACK |
        PipelineStateFlag::PIPELINE_STATE_FRONT_CLOCKWISE, 
        m_deferredRootSignature
    );

    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_0,
        m_denoisingRootSignature
    );
    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_1,
        m_denoisingRootSignature
    );
    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_2,
        m_denoisingRootSignature
    );
    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_4,
        m_denoisingRootSignature
    );
    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_8,
        m_denoisingRootSignature
    );
    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_16,
        m_denoisingRootSignature
    );
    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_RP,
        m_denoisingRootSignature
    );
    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_READBACK,
        m_denoisingRootSignature
    );
    m_graphicsMgr->CreatePipelineStateObject(
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
        PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_VARIANCE,
        m_denoisingRootSignature
    );


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(dxDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

    auto model = Dx12Model("assets\\gltf\\Room\\scene.gltf");
    m_scene = std::move(model.root);
    m_scene->SetMatrix(&GeoMath::Matrix4f::Scale(1.0f, -1.0f, 1.0f), nullptr, nullptr);

    m_cbvHeap        = std::move(model.cbvHeap);
    m_matConstBuffer = std::move(model.matConstBuffer);
    m_textures       = std::move(model.textures);

    m_rayTraceMeshInfoGpu = std::move(model.rayTraceMeshInfoGpu);
    m_rayTraceIndexBuffer = std::move(model.rayTraceIndexBuffer);
    m_rayTraceVertexBuffer = std::move(model.rayTraceVertexBuffer);

    m_bottomLevelAccelerationStructures = std::move(model.bottomLevelAccelerationStructures);

    float aspRatio = GetAspectRatio();
    m_camera = new Dx12Camera(0, m_scene.get());
    m_camera->SetLens(nullptr, nullptr, &aspRatio, nullptr);
    m_camera->SetMatrix(nullptr, nullptr, &GeoMath::Matrix4f::Translation(0.0, 5.0, -10.0f));

    m_scene->AddChild(std::unique_ptr<SceneNode>(m_camera));

    m_graphicsMgr->ExecuteCommandList(cmdList);
    m_graphicsMgr->Flush();

    cmdList = m_graphicsMgr->GetTempCommandList();
    {
        // Create Shader
        dxc::DxcDllSupport dxcDllHelper;
        ComPtr<IDxcCompiler> compiler;
        ComPtr<IDxcLibrary>  library;

        ThrowIfFailed(dxcDllHelper.Initialize());
        ThrowIfFailed(dxcDllHelper.CreateInstance(CLSID_DxcCompiler, compiler.GetAddressOf()));
        ThrowIfFailed(dxcDllHelper.CreateInstance(CLSID_DxcLibrary, library.GetAddressOf()));

        std::ifstream shaderFile("assets/shader/RayTracing.hlsl", std::ios::ate | std::ios::binary);
        assert(shaderFile.good(), "Can't open file " + wstring_2_string(std::wstring(filename)));

        size_t fileSize = static_cast<size_t>(shaderFile.tellg());
        std::vector<uint8_t> buffer(fileSize);

        shaderFile.seekg(0);
        shaderFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);

        // Create blob from the string
        ComPtr<IDxcBlobEncoding> pTextBlob;
        ComPtr<IDxcOperationResult> pResult;
        ThrowIfFailed(library->CreateBlobWithEncodingFromPinned(buffer.data(), fileSize, 0, pTextBlob.GetAddressOf()));

        ThrowIfFailed(compiler->Compile(
            pTextBlob.Get(), L"assets/shader/RayTracing.hlsl", L"", L"lib_6_3",
            nullptr, 0, nullptr, 0, nullptr, pResult.GetAddressOf()
        ));

        ComPtr<IDxcBlob> rayTracingLib;
        ThrowIfFailed(pResult->GetResult(rayTracingLib.GetAddressOf()));

        // Create Pipeline Object
        CD3DX12_STATE_OBJECT_DESC raytracingPipeline{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};
        {
            auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            lib->SetDXILLibrary(&CD3DX12_SHADER_BYTECODE(rayTracingLib->GetBufferPointer(), rayTracingLib->GetBufferSize()));
            lib->DefineExport(L"RayGen");
            lib->DefineExport(L"Hit");
            lib->DefineExport(L"Miss");
        }

        {
            auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitGroup->SetClosestHitShaderImport(L"Hit");
            hitGroup->SetHitGroupExport(L"HitGroup");
            hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
        }

        {
            auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
            shaderConfig->Config(4 * sizeof(float), 2 * sizeof(float));
        }

        {
            // Global root signature
            // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
            auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            globalRootSignature->SetRootSignature(m_rayTracingGlobalRootSignature.Get());
        }

        {
            auto hitLocalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            hitLocalRootSignature->SetRootSignature(m_hitLocalRootSignature.Get());

            auto hitRootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            hitRootSignatureAssociation->SetSubobjectToAssociate(*hitLocalRootSignature);
            hitRootSignatureAssociation->AddExport(L"Hit");
        }

        {
            auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
            UINT maxRecursionDepth = 2; 
            pipelineConfig->Config(maxRecursionDepth);

            auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            rootSignatureAssociation->SetSubobjectToAssociate(*pipelineConfig);
            rootSignatureAssociation->AddExport(L"RayGen");
            rootSignatureAssociation->AddExport(L"Hit");
            rootSignatureAssociation->AddExport(L"Miss");
        }

        auto CreateStateObject = [&](const D3D12_STATE_OBJECT_DESC desc){
            ThrowIfFailed(dxDevice->CreateStateObject(
                &desc, IID_PPV_ARGS(m_rayTracingStateObject.GetAddressOf())
            ));
        };

        CreateStateObject(raytracingPipeline);

        ComPtr<ID3D12StateObjectProperties> stateObjectProperties = nullptr;
        ThrowIfFailed(m_rayTracingStateObject->QueryInterface(IID_PPV_ARGS(stateObjectProperties.GetAddressOf())));

        uint32_t shaderTableSize = Utility::CalcAlignment<64>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        shaderTableSize += Utility::CalcAlignment<64>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        shaderTableSize += Utility::CalcAlignment<32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8) * m_textures.size();

        uint32_t byteOffset = 0;
        m_shaderTable = std::make_unique<UploadBuffer>(dxDevice, 1, shaderTableSize);

        m_shaderTable->CopyData(
            reinterpret_cast<uint8_t*>(stateObjectProperties->GetShaderIdentifier(L"RayGen")),
            D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, byteOffset
        );
        byteOffset += Utility::CalcAlignment<64>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        m_shaderTable->CopyData(
            reinterpret_cast<uint8_t*>(stateObjectProperties->GetShaderIdentifier(L"Miss")),
            D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, byteOffset
        );
        byteOffset += Utility::CalcAlignment<64>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        uint8_t* hitShader = reinterpret_cast<uint8_t*>(stateObjectProperties->GetShaderIdentifier(L"HitGroup"));
        uint32_t svvDescriptorSize = dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle = m_graphicsMgr->GetTexGpuHandle();
        for(auto& tex : m_textures){
            m_shaderTable->CopyData(hitShader, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, byteOffset);
            byteOffset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
            m_shaderTable->CopyData(reinterpret_cast<uint8_t*>(&texHandle), 8, byteOffset);
            byteOffset += Utility::CalcAlignment<32>(8);
            texHandle.Offset(1, svvDescriptorSize);
        }

        m_dispatchRayDesc.RayGenerationShaderRecord.StartAddress = m_shaderTable->GetGpuVirtualAddress();
        m_dispatchRayDesc.RayGenerationShaderRecord.SizeInBytes = Utility::CalcAlignment<32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        m_dispatchRayDesc.MissShaderTable.StartAddress = m_shaderTable->GetGpuVirtualAddress() + Utility::CalcAlignment<64>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        m_dispatchRayDesc.MissShaderTable.SizeInBytes = Utility::CalcAlignment<32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        m_dispatchRayDesc.MissShaderTable.StrideInBytes = Utility::CalcAlignment<32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        m_dispatchRayDesc.HitGroupTable.StartAddress = m_shaderTable->GetGpuVirtualAddress() + Utility::CalcAlignment<64>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) * 2;
        m_dispatchRayDesc.HitGroupTable.SizeInBytes = Utility::CalcAlignment<32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8) * m_textures.size();
        m_dispatchRayDesc.HitGroupTable.StrideInBytes = Utility::CalcAlignment<32>(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8);

    }

    m_graphicsMgr->ExecuteCommandList(cmdList);
    m_graphicsMgr->Flush();
}

void Pipeline::InitGUI(){

    auto dxDevice = m_graphicsMgr->GetDevice();
    D3D12_DESCRIPTOR_HEAP_DESC guiDescHeapDesc = {};
    guiDescHeapDesc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    guiDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    guiDescHeapDesc.NumDescriptors = 1;
    dxDevice->CreateDescriptorHeap(&guiDescHeapDesc, IID_PPV_ARGS(m_guiSrvDescHeap.GetAddressOf()));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(AppFramework::GetWnd());
    ImGui_ImplDX12_Init(dxDevice.Get(), 3,
        DXGI_FORMAT_R8G8B8A8_UNORM, m_guiSrvDescHeap.Get(),
        m_guiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        m_guiSrvDescHeap->GetGPUDescriptorHandleForHeapStart()
    );

}

void Pipeline::OnTick(){
    static const std::chrono::duration<double, std::ratio<1, 60>> deltaTime(1);
    static std::chrono::time_point<std::chrono::steady_clock> startTime;
    static std::chrono::time_point<std::chrono::steady_clock> endTime;

    endTime = std::chrono::steady_clock::now();
    if(endTime - startTime > deltaTime){
        startTime = std::chrono::steady_clock::now();
        OnUpdate();
        OnRender();
    }
}

void Pipeline::OnUpdate(){
    m_graphicsMgr->GetMainConstBuffer().alpha = m_alpha;
    m_scene->OnUpdate();
    m_graphicsMgr->OnUpdate();
}

void Pipeline::OnRender(){

    auto& currFrameRes = m_graphicsMgr->GetFrameResource();
    auto  cmdList      = m_graphicsMgr->GetCommandList();

    D3D12_VIEWPORT viewPorts[5] = {
        m_viewport, m_viewport, m_viewport,
        m_viewport, m_viewport
    };

    D3D12_RECT scissors[5] = {
        m_scissors, m_scissors, m_scissors,
        m_scissors, m_scissors
    };

    m_graphicsMgr->SetPipelineStateFlag(
        PipelineStateFlag::PIPELINE_STATE_RENDER_GBUFFER, 0x7000, false
    );
    cmdList->SetGraphicsRootSignature(m_deferredRootSignature.Get());
    cmdList->SetDescriptorHeaps(1, m_graphicsMgr->GetRTHeap().GetAddressOf());

    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->RSSetViewports(5, viewPorts);
    cmdList->RSSetScissorRects(5, scissors);

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    auto [rtvHandles, numGBuffer] = currFrameRes.gbuffer->AsRenderTarget(cmdList);
    cmdList->OMSetRenderTargets(numGBuffer, rtvHandles, FALSE, &dsvHandle);

    RenderScene();

    //Ray Tracing
    cmdList->SetPipelineState1(m_rayTracingStateObject.Get());
    cmdList->SetComputeRootSignature(m_rayTracingGlobalRootSignature.Get());
    cmdList->SetDescriptorHeaps(1, m_graphicsMgr->GetRTHeap().GetAddressOf());

    cmdList->SetComputeRootDescriptorTable(0, currFrameRes.uavGpuHandle[0]);
    cmdList->SetComputeRootConstantBufferView(1, currFrameRes.mainConst->GetGpuVirtualAddress());
    cmdList->SetComputeRootShaderResourceView(2, currFrameRes.topLevelAccelerationStructure->GetGPUVirtualAddress());
    cmdList->SetComputeRootShaderResourceView(3, m_rayTraceMeshInfoGpu->GetGpuVirtualAddress());
    cmdList->SetComputeRootShaderResourceView(4, m_matConstBuffer->GetGpuVirtualAddress());
    cmdList->SetComputeRootShaderResourceView(5, m_rayTraceIndexBuffer->GetGpuVirtualAddress());
    cmdList->SetComputeRootShaderResourceView(6, m_rayTraceVertexBuffer->GetGpuVirtualAddress());

    m_dispatchRayDesc.Width  = m_wndWidth;
    m_dispatchRayDesc.Height = m_wndHeight;
    m_dispatchRayDesc.Depth  = 1;

    cmdList->DispatchRays(&m_dispatchRayDesc);

    // Denoising
    if(m_openDenoising || m_openFrameBlend || m_openReprojection){
        auto& preFrame = m_graphicsMgr->GetPreFrameResource();
        cmdList->SetComputeRootSignature(m_denoisingRootSignature.Get());
        cmdList->SetComputeRootConstantBufferView(0, currFrameRes.mainConst->GetGpuVirtualAddress());
        cmdList->SetComputeRootDescriptorTable(1, currFrameRes.uavGpuHandle[0]);
        cmdList->SetComputeRootDescriptorTable(2, currFrameRes.uavGpuHandle[2]);
        cmdList->SetComputeRootDescriptorTable(3, preFrame.srvGpuHandle[0]);
        cmdList->SetComputeRootDescriptorTable(4, currFrameRes.srvGpuHandle[1]);
        cmdList->SetComputeRootDescriptorTable(
            5, currFrameRes.gbuffer->AsShaderResource(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        );

        if(m_openFrameBlend){
            if(m_openReprojection){
                m_graphicsMgr->SetPipelineStateFlag(
                    PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
                    PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_RP,
                    0xFFFFFFFF, true
                );
            }
            else{
                m_graphicsMgr->SetPipelineStateFlag(
                    PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
                    PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_0,
                    0xFFFFFFFF, true
                );
            }

            cmdList->Dispatch(m_wndWidth / 256 + 1, m_wndHeight, 1);
        }

        if(m_openDenoising){
            cmdList->SetComputeRootDescriptorTable(1, currFrameRes.uavGpuHandle[1]);
            cmdList->SetComputeRootDescriptorTable(4, currFrameRes.srvGpuHandle[0]);
            m_graphicsMgr->SetPipelineStateFlag(
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_1,
                0xFFFFFFFF, true
            );
            cmdList->Dispatch(m_wndWidth / 256 + 1, m_wndHeight, 1);

            m_graphicsMgr->SetPipelineStateFlag(
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_2,
                0x800F, true
            );
            cmdList->SetComputeRootDescriptorTable(1, currFrameRes.uavGpuHandle[0]);
            cmdList->SetComputeRootDescriptorTable(4, currFrameRes.srvGpuHandle[1]);
            cmdList->Dispatch(m_wndWidth / 256 + 1, m_wndHeight, 1);

            m_graphicsMgr->SetPipelineStateFlag(
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_4,
                0x800F, true
            );
            cmdList->SetComputeRootDescriptorTable(1, currFrameRes.uavGpuHandle[1]);
            cmdList->SetComputeRootDescriptorTable(4, currFrameRes.srvGpuHandle[0]);
            cmdList->Dispatch(m_wndWidth / 256 + 1, m_wndHeight, 1);

            m_graphicsMgr->SetPipelineStateFlag(
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_8,
                0x800F, true
            );
            cmdList->SetComputeRootDescriptorTable(1, currFrameRes.uavGpuHandle[0]);
            cmdList->SetComputeRootDescriptorTable(4, currFrameRes.srvGpuHandle[1]);
            cmdList->Dispatch(m_wndWidth / 256 + 1, m_wndHeight, 1);

            m_graphicsMgr->SetPipelineStateFlag(
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_16,
                0x800F, true
            );
            cmdList->SetComputeRootDescriptorTable(1, currFrameRes.uavGpuHandle[1]);
            cmdList->SetComputeRootDescriptorTable(4, currFrameRes.srvGpuHandle[0]);
            cmdList->Dispatch(m_wndWidth / 256 + 1, m_wndHeight, 1);

            m_graphicsMgr->SetPipelineStateFlag(
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
                PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_READBACK,
                0x800F, true
            );
            cmdList->SetComputeRootDescriptorTable(1, currFrameRes.uavGpuHandle[0]);
            cmdList->SetComputeRootDescriptorTable(4, currFrameRes.srvGpuHandle[1]);
            cmdList->Dispatch(m_wndWidth / 256 + 1, m_wndHeight, 1);
        }

    }
    else{
        m_graphicsMgr->SetPipelineStateFlag(
            PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_SHADER |
            PipelineStateFlag::PIPELINE_STATE_RENDER_COMPUTE_DENOISE_VARIANCE,
            0xFFFFFFFF, true
        );
        cmdList->SetComputeRootSignature(m_denoisingRootSignature.Get());

        cmdList->SetComputeRootConstantBufferView(0, currFrameRes.mainConst->GetGpuVirtualAddress());
        cmdList->SetComputeRootDescriptorTable(1, currFrameRes.uavGpuHandle[0]);
        cmdList->SetComputeRootDescriptorTable(2, currFrameRes.uavGpuHandle[2]);
        cmdList->SetComputeRootDescriptorTable(4, currFrameRes.srvGpuHandle[1]);
        cmdList->SetComputeRootDescriptorTable(
            5, currFrameRes.gbuffer->AsShaderResource(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        );
        cmdList->Dispatch(m_wndWidth / 256 + 1, m_wndHeight, 1);
    }

    // Render Screen
    // Render to back buffer
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        currFrameRes.renderTarget.Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    m_graphicsMgr->SetPipelineStateFlag(
        PipelineStateFlag::PIPELINE_STATE_SHADER_COMB_2  |
        PipelineStateFlag::PIPELINE_STATE_CULL_MODE_BACK |
        PipelineStateFlag::PIPELINE_STATE_FRONT_CLOCKWISE,
        0xFFFFFFFF, true
    );

    cmdList->SetGraphicsRootSignature(m_deferredRootSignature.Get());
    cmdList->SetGraphicsRootDescriptorTable(3, currFrameRes.gbuffer->AsShaderResource(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
    cmdList->SetGraphicsRootDescriptorTable(4, currFrameRes.srvGpuHandle[0]);

    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissors);

    cmdList->ClearRenderTargetView(currFrameRes.rtvHandle, m_backgroundColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &currFrameRes.rtvHandle, FALSE, nullptr);

    cmdList->IASetIndexBuffer(nullptr);
    cmdList->IASetVertexBuffers(0, 0, nullptr);
    cmdList->DrawInstanced(3, 1, 0, 0);

    RenderGUI();

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        currFrameRes.renderTarget.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
    ));

    m_graphicsMgr->OnRender();

}

void Pipeline::RenderScene(){
    auto& currFrameRes = m_graphicsMgr->GetFrameResource();
    auto  cmdList      = m_graphicsMgr->GetCommandList();
    
    cmdList->SetGraphicsRootConstantBufferView(1, currFrameRes.mainConst->GetGpuVirtualAddress()); 
    
    m_scene->OnRender();
}

void Pipeline::RenderGUI(){

    auto cmdList = m_graphicsMgr->GetCommandList();

    cmdList->SetDescriptorHeaps(1, m_guiSrvDescHeap.GetAddressOf());

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Control Panel");
        ImGui::ColorEdit3("Background Color", m_backgroundColor);
        ImGui::Separator();

        ImGui::Checkbox("Bilateral Filter", &m_openDenoising);
        ImGui::Checkbox("Temporal Blend", &m_openFrameBlend);
        ImGui::Checkbox("Reprojection", &m_openReprojection);
        ImGui::Separator();

        ImGui::SliderFloat("Alpha", &m_alpha, 0.0f, 1.0f);
        ImGui::Separator();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());
}

void Pipeline::OnDestroy(){
    m_graphicsMgr->Flush();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Pipeline::OnResize(const uint16_t width, const uint16_t height){

    m_graphicsMgr->Flush();
    auto& dxDevice = m_graphicsMgr->GetDevice();

    m_wndWidth  = max(1, width);
    m_wndHeight = max(1, height);

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width    = m_wndWidth;
    m_viewport.Height   = m_wndHeight;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.MinDepth = 0.0f;

    m_scissors.left   = 0.0f;
    m_scissors.top    = 0.0f;
    m_scissors.right  = m_wndWidth;
    m_scissors.bottom = m_wndHeight;

    float aspRatio = GetAspectRatio();
    m_camera->SetLens(nullptr, nullptr, &aspRatio, nullptr);
    
    // Create DepthStencil 
    {
        D3D12_RESOURCE_DESC dsResDesc = {};
        dsResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dsResDesc.Alignment = 0;
        dsResDesc.Width  = width;
        dsResDesc.Height = height;
        dsResDesc.DepthOrArraySize = 1;
        dsResDesc.MipLevels = 1;
        dsResDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsResDesc.SampleDesc.Count   = 1;
        dsResDesc.SampleDesc.Quality = 0;
        dsResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        dsResDesc.Flags  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValueDs = {};
        clearValueDs.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValueDs.DepthStencil.Depth   = 1.0f;
        clearValueDs.DepthStencil.Stencil = 0;

        m_depthStencil.Reset();
        dxDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
            &dsResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValueDs, IID_PPV_ARGS(m_depthStencil.GetAddressOf())
        );

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags         = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        dxDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, dsvHandle);
    }

    m_graphicsMgr->OnResize(m_wndWidth, m_wndHeight);

}

void Pipeline::OnKeyDown(const uint16_t key, const int16_t x, const int16_t y){

    switch(key){
        case WM_LBUTTONDOWN:
            m_isLeftMouseDown = true;
            break;
        case WM_MOUSEMOVE:
        {
            auto& io = ImGui::GetIO();
            if(m_isLeftMouseDown && !io.WantCaptureKeyboard && !io.WantCaptureMouse){
                m_camera->Input(key, x - m_lastMousePosX, y - m_lastMousePosY);
            }
            m_lastMousePosX = x;
            m_lastMousePosY = y;
            break;
        }
        case WM_KEYDOWN:
            m_camera->Input(x);
            break;
        default:
            break;
    }

}

void Pipeline::OnKeyUp(const uint16_t key, const int16_t x, const int16_t y){
    switch(key){
        case WM_LBUTTONUP:
            m_isLeftMouseDown = false;
            break;

        default:
            break;
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT Pipeline::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){

    if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)){
        return true;
    }

    switch (msg){
        case WM_SIZE:
        {
            OnResize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            float t = GET_WHEEL_DELTA_WPARAM(wParam);
            OnKeyDown(static_cast<uint16_t>(WM_MOUSEWHEEL), GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            OnKeyDown(static_cast<uint16_t>(WM_MOUSEMOVE), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_LBUTTONDOWN:
        {
            SetCapture(hWnd);
            OnKeyDown(static_cast<uint16_t>(WM_LBUTTONDOWN), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_LBUTTONUP:
        {
            OnKeyUp(static_cast<uint16_t>(WM_LBUTTONUP), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            ReleaseCapture();
            return 0;
        }
        case WM_KEYDOWN:
        {
            OnKeyDown(static_cast<uint16_t>(WM_KEYDOWN), static_cast<uint16_t>(wParam));
            return 0;
        }
        case WM_KEYUP:
        {
            OnKeyUp(static_cast<uint16_t>(WM_KEYUP), static_cast<uint16_t>(wParam));
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        default:
        {
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }
}

