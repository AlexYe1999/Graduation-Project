#include "Denoising.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID){

    if(dispatchThreadID.x < mainConst.resolution.x){
        finalResult[dispatchThreadID.xy] = newRenderResult[dispatchThreadID.xy];
    }
}