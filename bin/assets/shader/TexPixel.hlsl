#include "DeferredShading.hlsli"

GBuffer main(TexPixel psIn){
    GBuffer gbuffer;
    gbuffer.baseColor = matConst.baseColor * tex0.Sample(anisoSampler, psIn.texCoord);
    gbuffer.position  = float4(psIn.posW, psIn.posH.z);
    gbuffer.normal    = float4((psIn.normalW+1.0f)*0.5f, 1.0f);
    gbuffer.misc      = float4(psIn.posH.z, 0.0f, 0.0f, 1.0f);
    gbuffer.objectID  = objConst.objectIndex;
    return gbuffer;
}