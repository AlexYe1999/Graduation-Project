#include "Denoising.hlsli"
[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID){
    
 const float weights[5] = {0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f};

    if(dispatchThreadID.x < mainConst.resolution.x){  
        float3 preVariance = varianceResult[dispatchThreadID.xy].xyz;
        float3 color_p     = newRenderResult[dispatchThreadID.xy].xyz;
        float3 normal_p    = normalize(newNormal.Load(uint3(dispatchThreadID.xy, 0), 0).xyz);
        float  depth_p     = newPosition.Load(uint3(dispatchThreadID.xy, 0), 0).w;
        uint   objectID_p  = newObjectID.Load(uint3(dispatchThreadID.xy, 0), 0);

        float2 gradient; 
        if(dispatchThreadID.x+1 < mainConst.resolution.x){
            gradient.x = newPosition.Load(uint3(dispatchThreadID.x+1, dispatchThreadID.y, 0), 0).w - depth_p; 
        }
        else{
            gradient.x = depth_p - newPosition.Load(uint3(dispatchThreadID.x-1, dispatchThreadID.y, 0), 0).w;
        }

        if(dispatchThreadID.y+1 < mainConst.resolution.y){
            gradient.y = newPosition.Load(uint3(dispatchThreadID.x, dispatchThreadID.y+1, 0), 0).w - depth_p; 
        }
        else{
            gradient.y = depth_p - newPosition.Load(uint3(dispatchThreadID.x, dispatchThreadID.y-1, 0), 0).w; 
        }

        float3 gaussianResult = 0.0f;
        float3 tmpWeight = 1e-20f;
        for(int offsetX = -1; offsetX <= 1; offsetX++){
            for(int offsetY = -1; offsetY <= 1; offsetY++){
                if(dispatchThreadID.x+offsetX >= 0 && dispatchThreadID.x+offsetX < mainConst.resolution.x
                && dispatchThreadID.y+offsetY >= 0 && dispatchThreadID.y+offsetY < mainConst.resolution.y
                ){
                    float3 factor = GaussianFactor(offsetX, offsetY, preVariance);
                    gaussianResult += factor * newRenderResult.Load(uint3(dispatchThreadID.x+offsetX,dispatchThreadID.y+offsetY, 0), 0).xyz;
                    tmpWeight += factor;
                }
            }
        }

        gaussianResult /= tmpWeight;

        float3 result = 0.0f;
        float3 variance = 0.0f;
        float3 totalWeight = 1e-20f;
        for(int i = 0; i < 33; i+=8){
            for(int j = 0; j < 33; j+=8){
                const int2 index = int2(dispatchThreadID.x+i-8, dispatchThreadID.y+j-8);
                if(0 <= index.x && index.x < (int)mainConst.resolution.x && 0 <= index.y && index.y < (int)mainConst.resolution.y){
                    if(objectID_p == newObjectID.Load(uint3(index, 0), 0)){

                        float3 color_q  = newRenderResult[index].xyz;
                        float3 normal_q = normalize(newNormal.Load(uint3(index, 0), 0).xyz);
                        float  depth_q  = newPosition.Load(uint3(index, 0), 0).w;

                        float2 deltaIndex = index-dispatchThreadID.xy;
                        float weight_depth  = exp(-abs(depth_p-depth_q) / abs(dot(gradient, deltaIndex)+1e-20));
                        float weight_normal = pow(max(0.0f, dot(normal_p, normal_q)), 128.0f);
                        float3 weight_color  = exp(-abs(color_p-color_q)/(4.0f*sqrt(1.0f)+1e-20));

                        float3 weight = weights[i]*weights[j]*weight_depth*weight_normal*weight_color;
                        result += weight*color_q;
                        variance += weight*weight*preVariance;
                        totalWeight += weight;
                    }
                }
            }
        }

        finalResult[dispatchThreadID.xy] = float4(result / totalWeight, 1.0f);
        varianceResult[dispatchThreadID.xy] = float4(variance / pow(totalWeight, 2.0f), 1.0f);
    }
}