#include"DeferredShading.hlsli"

float4 main(float4 posViewPort : SV_POSITION) : SV_Target{
    return tex3.Load(uint3(posViewPort.xy, 0), 0);
}