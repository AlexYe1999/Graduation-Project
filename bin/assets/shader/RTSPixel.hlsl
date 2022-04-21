#include "DeferredShading.hlsli"

float4 main(float4 posViewPort : SV_POSITION) : SV_Target{
    return pow(tex0.Load(uint3(posViewPort.xy, 0), 0) * luminance.Load(uint3(posViewPort.xy, 0), 0),1.0/2.2f);
}