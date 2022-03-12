#pragma once
#include "GeoMath.hpp"
#include "Texture.hpp"

struct MaterialConstant{

    MaterialConstant(){}

    // float4
    float alphaCutoff = 0.5f;
    GeoMath::Vector3f padding{0.0f, 0.0f, 0.0f};

    // float4
    uint16_t materialType = 0x0000;
    uint16_t textureMask  = 0x0000;
    GeoMath::Vector3f essisiveFactor{0.0f, 0.0f, 0.0f};

    // Workflow Parameter
    // normal    => texture0
    // occlusion => texture1
    // essisive  => texture2
   
    // float4
    // BasicTex  => texture3 
    union{
        GeoMath::Vector4f baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f}; 
        GeoMath::Vector4f diffuseFactor;                           
    };

    //float4
    union{
        GeoMath::Vector4f miscVector{0.0f, 0.0f, 0.0f, 0.0f};
        
        // Material PBR Metallic Roughness => texture4
        struct{
            float metallicFactor;
            float roughnessFactor;
        };

        // KHR_materials_pbrSpecularGlossiness => texture4 texture5 ?
        struct{ 
            GeoMath::Vector3f specularFactor;                     
            float glossinessFactor;
        };
    };

    // float12
    GeoMath::Vector2f normalTexScale{1.0f, 1.0f};
    GeoMath::Vector2f occlusionTexScale{1.0f, 1.0f};
    GeoMath::Vector2f essisiveTexScale{1.0f, 1.0f};
    GeoMath::Vector2f basicTexScale{1.0f, 1.0f};
    GeoMath::Vector2f tex4TexScale{1.0f, 1.0f};
    GeoMath::Vector2f tex5TexScale{1.0f, 1.0f};
};

static_assert(sizeof(MaterialConstant) == 112);


class Material{
public:
    Material(){}
    virtual void OnRender() = 0;
};