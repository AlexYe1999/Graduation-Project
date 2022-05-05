#include "Denoising.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID){
    if(dispatchThreadID.x < mainConst.resolution.x){
        float4 pos = mul(float4(newPosition[dispatchThreadID.xy].xyz, 1.0f), mul(mainConst.preView, mainConst.proj));
        pos.y *= -1.0f;
        
        float2 screan = (float2(pos.xy / pos.w) + 1.0f) * 0.5f;
        if(screan.x > 0 && screan.y > 0 && screan.x < 1.0f && screan.y < 1.0f){
            screan*= mainConst.resolution;
            finalResult[dispatchThreadID.xy] = (1.0f-mainConst.alpha)*preRenderResult[screan] + mainConst.alpha*newRenderResult[dispatchThreadID.xy];
        }
        else{
            finalResult[dispatchThreadID.xy] = newRenderResult[dispatchThreadID.xy];
        }  
    }
}