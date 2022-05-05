#include "Denoising.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID){
    if(dispatchThreadID.x < mainConst.resolution.x){
        finalResult[dispatchThreadID.xy] = (1.0f-mainConst.alpha)*preRenderResult[dispatchThreadID.xy] + mainConst.alpha*newRenderResult[dispatchThreadID.xy];
    }
}