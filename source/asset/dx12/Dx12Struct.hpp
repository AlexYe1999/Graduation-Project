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


struct Vertex0 : public rtti::SOA{
    rtti::Var<GeoMath::Vector3f> position = {"POSITION", 0};
    rtti::Var<GeoMath::Vector3f> normal   = {"NORMAL",   0};
};

struct Vertex1 : public rtti::SOA{
    rtti::Var<GeoMath::Vector3f> position = {"POSITION", 0};
    rtti::Var<GeoMath::Vector3f> normal   = {"NORMAL",   0};
    rtti::Var<GeoMath::Vector4f> tangent  = {"TANGENT",  0};
    rtti::Var<GeoMath::Vector2f> texcoord = {"TEXCOORD", 0};
};

struct MainConstBuffer{
    GeoMath::Matrix4f m_mView;
    GeoMath::Matrix4f m_mProj;
    GeoMath::Vector3f m_vCameraPos;
    float pad;
};

struct ObjectConst{
    GeoMath::Matrix4f toWorld;
    GeoMath::Matrix4f toLocal;
};