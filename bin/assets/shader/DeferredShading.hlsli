
struct MainFrameConstants{
    matrix model;
    matrix view;
    matrix proj;
    float4 cameraPosition;
    float  fov;
};

struct ObjectConstants{
    matrix toWorld;
    matrix toLocal;
    uint   objectIndex;   
};

struct MaterialConstants{
    float  alphaCutoff;
    float3 padding;

    uint   miscMask;
    float3 essisiveFactor;
    float4 baseColor;
    float4 miscVector;

    float2 normalTexScale;
    float2 occlusionTexScale;
    float2 essisiveTexScale;
    float2 basicTexScale;
    float2 tex4TexScale;
    float2 tex5TexScale;
};

ConstantBuffer<MainFrameConstants> mainConst : register(b0);
ConstantBuffer<ObjectConstants>    objConst  : register(b1);
ConstantBuffer<MaterialConstants>  matConst  : register(b2);

SamplerState pointSampler : register(s0);
SamplerState anisoSampler : register(s1);

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
    float4 posH      : SV_POSITION;
    float3 posW      : POSITION;
    float3 normalW   : NORMAL;
    float4 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
};

struct GBuffer{
    float4 baseColor : SV_TARGET0;
    float4 position  : SV_TARGET1;
    float4 normal    : SV_TARGET2;
    float4 misc      : SV_TARGET3;
    uint   objectID  : SV_TARGET4;
};