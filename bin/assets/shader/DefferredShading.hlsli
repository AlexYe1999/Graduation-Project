
cbuffer MainBuffer : register(b0){
    matrix gView;
    matrix gProj;
    float4 gCameraPos;
};

cbuffer ObjBuffer : register(b1){
    matrix gToWorld;
    matrix gToLocal;
};

SamplerState gSampler : register(s0);

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