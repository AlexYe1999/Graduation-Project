#include "Denoising.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID){
    if(dispatchThreadID.x < mainConst.resolution.x){
        finalResult[dispatchThreadID.xy] = 0.2f*preRenderResult[dispatchThreadID.xy] + 0.8f*newRenderResult[dispatchThreadID.xy];
    }
}