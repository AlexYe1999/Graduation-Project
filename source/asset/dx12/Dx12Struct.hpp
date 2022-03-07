#pragma once
#include "ReflectableStruct.hpp"
#include "GeoMath.hpp"

template<>
struct rtti::Var<GeoMath::Vector2f> : public rtti::VarType<GeoMath::Vector2f>{
    Var(char const* semantic, uint8_t semanticIndex) : VarType<GeoMath::Vector2f>(rtti::VarTypeData{rtti::VarTypeData::ScaleType::Float, 2, semanticIndex, semantic}){};
};

template<>
struct rtti::Var<GeoMath::Vector3f> : public rtti::VarType<GeoMath::Vector3f>{
    Var(char const* semantic, uint8_t semanticIndex) : VarType<GeoMath::Vector3f>(rtti::VarTypeData{rtti::VarTypeData::ScaleType::Float, 3, semanticIndex, semantic}){};
};

template<>
struct rtti::Var<GeoMath::Vector4f> : public rtti::VarType<GeoMath::Vector4f>{
    Var(char const* semantic, uint8_t semanticIndex) : VarType<GeoMath::Vector4f>(rtti::VarTypeData{rtti::VarTypeData::ScaleType::Float, 4, semanticIndex, semantic}){};
};

struct Dx12SOA : public rtti::SOA{
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
protected:
    void GetInputLayout(){

        auto GetDxFormat = [&](rtti::VarTypeData typeData)->DXGI_FORMAT{

            switch(typeData.scale){
                case rtti::VarTypeData::ScaleType::Float:
                    switch(typeData.dimension){
                        case 1:
                            return DXGI_FORMAT_R32_FLOAT;
                        case 2:
                            return DXGI_FORMAT_R32G32_FLOAT;
                        case 3:
                            return DXGI_FORMAT_R32G32B32_FLOAT;
                        case 4:
                            return DXGI_FORMAT_R32G32B32A32_FLOAT;
                        default:
                            return DXGI_FORMAT_UNKNOWN;
                    }
                case rtti::VarTypeData::ScaleType::Int:
                    switch(typeData.dimension){
                        case 1:
                            return DXGI_FORMAT_R32_SINT;
                        case 2:
                            return DXGI_FORMAT_R32G32_SINT;
                        case 3:
                            return DXGI_FORMAT_R32G32B32_SINT;
                        case 4:
                            return DXGI_FORMAT_R32G32B32A32_SINT;
                        default:
                            return DXGI_FORMAT_UNKNOWN;
                    }
                case rtti::VarTypeData::ScaleType::UInt:
                    switch(typeData.dimension){
                        case 1:
                            return DXGI_FORMAT_R32_UINT;
                        case 2:
                            return DXGI_FORMAT_R32G32_UINT;
                        case 3:
                            return DXGI_FORMAT_R32G32B32_UINT;
                        case 4:
                            return DXGI_FORMAT_R32G32B32A32_UINT;
                        default:
                            return DXGI_FORMAT_UNKNOWN;
                    }

                default:
                    return DXGI_FORMAT_UNKNOWN;
            }
        };

        inputLayout.resize(m_variables.size());

        for(size_t index = 0; index < m_variables.size(); index++){
            auto& data = m_variables[index];
            auto& desc = inputLayout[index];
            desc.SemanticName   = data.semantic;
            desc.SemanticIndex  = data.semanticIndex;
            desc.Format         = GetDxFormat(data);
            desc.InputSlot      = index;
            desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            desc.InstanceDataStepRate = 0;
        }
    };

};

struct Vertex0 : public Dx12SOA{
    Vertex0(){
        GetInputLayout();
    }
    const rtti::Var<GeoMath::Vector3f> position = {"POSITION", 0};
    const rtti::Var<GeoMath::Vector3f> normal   = {"NORMAL",   0};
};

struct Vertex1 : public Dx12SOA{
    Vertex1(){
        GetInputLayout();
    }
    const rtti::Var<GeoMath::Vector3f> position = {"POSITION", 0};
    const rtti::Var<GeoMath::Vector3f> normal   = {"NORMAL",   0};
    const rtti::Var<GeoMath::Vector4f> tangent  = {"TANGENT",  0};
    const rtti::Var<GeoMath::Vector2f> texCoord = {"TEXCOORD", 0};
};

struct MainConstBuffer{
    GeoMath::Matrix4f view;
    GeoMath::Matrix4f proj;
    GeoMath::Vector4f cameraPos;
};

struct ObjectConstBuffer{
    GeoMath::Matrix4f toWorld;
    GeoMath::Matrix4f toLocal;
};

struct MatConstBuffer{};