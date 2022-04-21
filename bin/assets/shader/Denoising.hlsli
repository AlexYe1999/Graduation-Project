struct MainFrameConstants{
    matrix model;
    matrix view;
    matrix proj;
    float4 cameraPosition;
    float2 randomSeed;
    uint2  resolution;
    float  fov;
    float  time;
    float2 padding;
    matrix preView;
    matrix preProj;
};

ConstantBuffer<MainFrameConstants> mainConst : register(b0);

RWTexture2D<float4> finalResult    : register(u0, space3);
RWTexture2D<float4> varianceResult : register(u1, space3);
Texture2D<float4> preRenderResult  : register(t0, space3);
Texture2D<float4> newRenderResult  : register(t1, space3);

Texture2D<float4> newAlbedo        : register(t2, space3);
Texture2D<float4> newPosition      : register(t3, space3);
Texture2D<float4> newNormal        : register(t4, space3);
Texture2D<float4> newMisc          : register(t5, space3);
Texture2D<uint>   newObjectID      : register(t6, space3);

float3 GaussianFactor(int x, int y, float3 sigma){
    return 0.5f / (3.1415926f * sigma) * exp(-0.5f*(x*x+y*y)/(sigma+1e-20));
}