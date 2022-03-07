#include"DefferredShading.hlsli"

BasicPixel main(BasicVertex vsIn){
    BasicPixel vsOut;
    float4 world = mul(float4(vsIn.posL, 1.0f), gToWorld);

    vsOut.posW = world.xyz;
    vsOut.posH = mul(world, gView);
    vsOut.posH = mul(vsOut.posH, gProj);

    vsIn.normalL   = normalize(vsIn.normalL);
    vsOut.normalW  = normalize(mul(vsIn.normalL, transpose((float3x3)gToLocal)));

    return vsOut;
}