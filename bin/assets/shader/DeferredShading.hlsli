
cbuffer MainBuffer : register(b0){
    matrix gView;
    matrix gProj;
    float4 gCameraPos;
};

cbuffer ObjBuffer : register(b1){
    matrix gToWorld;
    matrix gToLocal;
};

cbuffer MatBuffer : register(b2){
    float  gAlphaCutoff;
    float3 padding;

    uint   gMiscMask;
    float3 gEssisiveFactor;
    float4 gBaseColor;
    float4 gMiscVector;

    float2 gNormalTexScale;
    float2 gOcclusionTexScale;
    float2 gEssisiveTexScale;
    float2 gBasicTexScale;
    float2 gTex4TexScale;
    float2 gTex5TexScale;
};

SamplerState gPointSampler  : register(s0);
SamplerState gAnisoSampler  : register(s1);

Texture2D<float4> tex0 : register(t0);
Texture2D<float4> tex1 : register(t1);
Texture2D<float4> tex2 : register(t2);
Texture2D<float4> tex3 : register(t3);
Texture2D<uint>   tex4 : register(t4);

struct BasicVertex{
    float3 posL    : POSITION;
    float3 normalL : NORMAL;
};

struct BasicPixel{
    float4 posH    : SV_POSITION;
    float3 posW    : POSITION;
    float3 normalW : NORMAL;
};

struct TexVertex{
    float3 posL     : POSITION;
    float3 normalL  : NORMAL;
    float4 tangent  : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct TexPixel{
    float4 posH    : SV_POSITION;
    float3 posW    : POSITION;
    float3 normalW : NORMAL;
};

struct GBuffer{
    float4 baseColor : SV_Target0;
    float4 emissive  : SV_Target1;
    float4 position  : SV_Target2;
    float4 normal    : SV_Target3;
    uint   objectID  : SV_Target4;
};