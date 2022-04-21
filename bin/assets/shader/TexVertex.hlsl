#include"DeferredShading.hlsli"

TexPixel main(TexVertex vsIn){
    TexPixel vsOut;
    float4 world = mul(float4(vsIn.posL, 1.0f), objConst.toWorld);

    vsOut.posW = world.xyz;
    vsOut.posH = mul(world, mainConst.view);
    vsOut.posH = mul(vsOut.posH, mainConst.proj);
    
    vsIn.normalL   = normalize(vsIn.normalL);
    vsOut.normalW  = normalize(mul(vsIn.normalL, transpose((float3x3)objConst.toLocal)));
    vsOut.tangent  = vsIn.tangent;
    vsOut.texCoord = vsIn.texCoord;
    return vsOut;
}