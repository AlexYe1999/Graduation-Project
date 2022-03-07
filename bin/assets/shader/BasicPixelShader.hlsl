#include "DefferredShading.hlsli"

float4 main(BasicPixel psIn) : SV_Target{
    return float4((psIn.normalW+float3(1.0f,1.0f,1.0f))*0.5f, 1.0f);
}