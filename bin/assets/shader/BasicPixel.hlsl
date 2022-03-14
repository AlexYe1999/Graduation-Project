#include "DeferredShading.hlsli"

GBuffer main(BasicPixel psIn){
    GBuffer gbuffer;
    gbuffer.baseColor = gBaseColor;
    gbuffer.emissive  = float4(gEssisiveFactor, 1.0f);
    gbuffer.position  = float4(psIn.posW, 1.0f);
    gbuffer.normal    = float4((psIn.normalW+1.0f)*0.5f, 1.0f);
    gbuffer.objectID  = 0;
    return gbuffer;
}