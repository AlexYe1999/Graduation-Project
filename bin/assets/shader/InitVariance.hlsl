#include "Denoising.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID){

    if(dispatchThreadID.x < mainConst.resolution.x){
        float3 normal_p = normalize(newNormal.Load(uint3(dispatchThreadID.xy, 0), 0).xyz);
        float  depth_p  = newPosition.Load(uint3(dispatchThreadID.xy, 0), 0).w;
        
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

        float3 mu = 0.0f;
        float3 mu_2 = 0.0f;
        float weight = 1e-20;
        for(int offsetX = -3; offsetX <= 3; offsetX++){
            for(int offsetY = -3; offsetY <= 3; offsetY++){
                if(dispatchThreadID.x+offsetX >= 0 && dispatchThreadID.x+offsetX < mainConst.resolution.x
                && dispatchThreadID.y+offsetY >= 0 && dispatchThreadID.y+offsetY < mainConst.resolution.y
                ){
                    float3 normal_q = normalize(newNormal.Load(uint3(dispatchThreadID.x+offsetX, dispatchThreadID.y+offsetY, 0), 0).xyz);
                    float  depth_q  = newPosition.Load(uint3(dispatchThreadID.x+offsetX, dispatchThreadID.y+offsetY, 0), 0).w;
                
                    float2 deltaIndex = float2(offsetX, offsetY);
                    float weight_depth  = exp(-abs(depth_p-depth_q) / abs(dot(gradient, deltaIndex)+1e-20));
                    float weight_normal = pow(max(0.0f, dot(normal_p, normal_q)), 128.0f);

                    float3 color = newRenderResult[float2(dispatchThreadID.x+offsetX, dispatchThreadID.y+offsetY)].xyz;
                    mu += color * weight_depth * weight_normal;
                    mu_2 += color * color * weight_depth * weight_normal;
                    weight += weight_depth * weight_normal;
                }
            }
        }

        mu /= weight;
        mu_2 /= weight;
        varianceResult[dispatchThreadID.xy] = float4(max(mu_2-pow(mu, 2.0f), 1e-20), 1.0f);
    }
}