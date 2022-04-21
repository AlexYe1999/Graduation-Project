struct MainFrameConstants{
    matrix model;
    matrix view;
    matrix proj;
    float4 cameraPosition;
    float2 randomSeed;
    uint2  resolution;
    float  fov;
    float  time;
};

struct MaterialConstants{
    float  alphaCutoff;
    float3 padding;

    uint   miscMask;
    float3 essisiveFactor;
    float4 baseColor;
    float4 miscVector;

    float2 normalTexScale;
    float2 occlusionTexScale;
    float2 essisiveTexScale;
    float2 basicTexScale;
    float2 tex4TexScale;
    float2 tex5TexScale;
};

struct RayTraceMeshInfo{
    uint indexOffsetBytes;
    uint positionOffsetBytes;
    uint normalOffsetBytes;
    uint tangentOffsetBytes;
    uint uvOffsetBytes;
    uint matIndex;
};

struct Payload{
    float3 color;
    uint   rayType;
};

SamplerState pointSampler : register(s2);
SamplerState anisoSampler : register(s3);

RWTexture2D<float4> renderTarget : register(u0, space1);
ConstantBuffer<MainFrameConstants> mainConst : register(b0, space1);

RaytracingAccelerationStructure     scene    : register(t0, space1);
StructuredBuffer<RayTraceMeshInfo>  meshInfo : register(t1, space1);
StructuredBuffer<MaterialConstants> matConst : register(t2, space1);

ByteAddressBuffer indices     : register(t3, space1);
ByteAddressBuffer attributes  : register(t4, space1);

float3 RayPlaneIntersection(float3 planeOrigin, float3 planeNormal, float3 rayOrigin, float3 rayDirection){
    float t = dot(-planeNormal, rayOrigin - planeOrigin) / dot(planeNormal, rayDirection);
    return rayOrigin + rayDirection * t;
}

float3 BarycentricCoordinates(float3 pt, float3 v0, float3 v1, float3 v2){
    float3 e0 = v1 - v0;
    float3 e1 = v2 - v0;
    float3 e2 = pt - v0;
    float d00 = dot(e0, e0);
    float d01 = dot(e0, e1);
    float d11 = dot(e1, e1);
    float d20 = dot(e2, e0);
    float d21 = dot(e2, e1);
    float denom = 1.0 / (d00 * d11 - d01 * d01);
    float v = (d11 * d20 - d01 * d21) * denom;
    float w = (d00 * d21 - d01 * d20) * denom;
    float u = 1.0 - v - w;
    return float3(u, v, w);
}

float3 GetCameraRay(in uint2 screen){
    float2 dim = DispatchRaysDimensions();
    float2 xy = screen + float2(0.5f, 0.5f);
    float height = tan(mainConst.fov * 0.5f);
    float width  = height * dim.x / dim.y;

    float4 screenPos = float4(xy / dim.xy * 2.0f - 1.0f, 1.0f, 1.0f);
    screenPos.xy *= float2(width, -height);

    float3 world  = mul(screenPos, mainConst.model);
    float3 origin = mainConst.cameraPosition.xyz;

    return normalize(world - origin);
}

float NDF_GGX(float NdotH, float roughness){
    float a  = roughness * roughness; 
    float a2 = a * a;
    float demon = NdotH * NdotH * (a2 - 1) + 1;
    return a2 / (3.1415926f * demon * demon);
}

float3 Freshnel_Schlick(float3 F0, float VdotH){
    return F0 + (float3(1.0f, 1.0f, 1.0f) - F0)*pow(1-VdotH, 5);
}

float Vis_Schlick(float NdotV, float NdotL, float roughness){
    float k = roughness * roughness * 0.5;
    float visSchlickV = NdotV * (1-k) + k;
    float visSchlickL = NdotL * (1-k) + k;
    return 0.25 / (visSchlickV * visSchlickL);
}

// based on https://github.com/keijiro/NoiseShader/blob/master/Assets/GLSL/SimplexNoise2D.glsl
// which itself is modification of https://github.com/ashima/webgl-noise/blob/master/src/noise3D.glsl
// 
// License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//           Distributed under the MIT License. See LICENSE file.
//           https://github.com/keijiro/NoiseShader/blob/master/LICENSE
//           https://github.com/ashima/webgl-noise
//           https://github.com/stegu/webgl-noise
float3 mod289(float3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float2 mod289(float2 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float3 permute(float3 x) {
    return mod289((x * 34.0 + 1.0) * x);
}

float3 taylorInvSqrt(float3 r) {
    return 1.79284291400159 - 0.85373472095314 * r;
}

// output noise is in range [-1, 1]
float snoise(float2 v) {
    const float4 C = float4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                            0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                            -0.577350269189626, // -1.0 + 2.0 * C.x
                            0.024390243902439); // 1.0 / 41.0

    // First corner
    float2 i  = floor(v + dot(v, C.yy));
    float2 x0 = v - i + dot(i, C.xx);

    // Other corners
    float2 i1;
    i1.x = step(x0.y, x0.x);
    i1.y = 1.0 - i1.x;

    // x1 = x0 - i1  + 1.0 * C.xx;
    // x2 = x0 - 1.0 + 2.0 * C.xx;
    float2 x1 = x0 + C.xx - i1;
    float2 x2 = x0 + C.zz;

    // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    float3 p =
      permute(permute(i.y + float3(0.0, i1.y, 1.0))
                    + i.x + float3(0.0, i1.x, 1.0));

    float3 m = max(0.5 - float3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0);
    m = m * m;
    m = m * m;

    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    float3 x = 2.0 * frac(p * C.www) - 1.0;
    float3 h = abs(x) - 0.5;
    float3 ox = floor(x + 0.5);
    float3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    m *= taylorInvSqrt(a0 * a0 + h * h);

    // Compute final noise value at P
    float3 g = float3(
        a0.x * x0.x + h.x * x0.y,
        a0.y * x1.x + h.y * x1.y,
        g.z = a0.z * x2.x + h.z * x2.y
    );
    return 130.0 * dot(m, g);
}

float snoise01(float2 v) {
    return snoise(v) * 0.5 + 0.5;
}

float3 RandomRay(float2 v) {
    float cosTheta = snoise01(v);
    float phi      = snoise01(v) * 2.0f * 3.1415926f;
    float r        = sqrt(max(0.0f, 1.0f - cosTheta*cosTheta));
    return float3(r * cos(phi), r * sin(phi), cosTheta);
}

[shader("raygeneration")]
void RayGen(){

    RayDesc rayDesc = {
        mainConst.cameraPosition.xyz,
        0.0f,
        GetCameraRay(DispatchRaysIndex().xy),
        asfloat(0x7F7FFFFF)
    };

    Payload payload;
    payload.rayType = 0;
    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, rayDesc, payload);

    renderTarget[DispatchRaysIndex().xy] = float4(payload.color, 1.0f);
}

Texture2D<float4> essisiveTex : register(t0, space2);

[shader("closesthit")]
void Hit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr){
    RayTraceMeshInfo info = meshInfo[InstanceID()]; 
    MaterialConstants mat = matConst[info.matIndex];
    
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    const uint indexOffset = info.indexOffsetBytes;
    const uint posOffset   = info.positionOffsetBytes;   
    const uint3 ii = indices.Load3(indexOffset+PrimitiveIndex()*12);

    if(payload.rayType == 0){

        const float roughness = 1.0f - mat.miscVector.w;
        float specular = max(mat.miscVector.x, mat.miscVector.y);
        specular = max(specular, mat.miscVector.z);
        specular = min(specular, 0.8f); 

        const float3 rayDirection = WorldRayDirection();
        const float3 origin = WorldRayOrigin() + rayDirection * RayTCurrent();

        const float3 normal0 = asfloat(attributes.Load3(info.normalOffsetBytes+ii.x*12));
        const float3 normal1 = asfloat(attributes.Load3(info.normalOffsetBytes+ii.y*12));
        const float3 normal2 = asfloat(attributes.Load3(info.normalOffsetBytes+ii.z*12));
        const float3 p0 = mul(float4(asfloat(attributes.Load3(info.positionOffsetBytes+ii.x*12)), 1.0f), ObjectToWorld4x3()).xyz;
        
        float3 normal = normalize(mul(bary.x * normal0 + bary.y * normal1 + bary.z * normal2, (float3x3)WorldToObject3x4()));
        if(dot(rayDirection, normal) > 0.0f){
            normal = -normal;
        }
        float3 tangent   = normalize(p0-origin);
        float3 bitangent = cross(normal, tangent);

        float2 randomNum = attr.barycentrics*mainConst.randomSeed;
        float3 randomRay = normalize(mul(RandomRay(randomNum), float3x3(tangent, bitangent, normal)));

        RayDesc rayDesc = {
            origin,
            0.01f, 
            randomRay,
            asfloat(0x7F7FFFFF)
        };

        Payload nextPayload;
        nextPayload.rayType = 1;
        TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, rayDesc, nextPayload);

        float3 halfVector = normalize(randomRay-rayDirection);
        float NdotH = dot(normal, halfVector);
        float VdotH = dot(-rayDirection, halfVector);
        float NdotV = dot(normal, -rayDirection);
        float NdotL = dot(normal, randomRay);
        
        payload.color = 2.0f * (nextPayload.color - nextPayload.color*specular
            + specular * 3.1415926f * Freshnel_Schlick(mat.miscVector.bgr, VdotH) * NDF_GGX(NdotH, roughness) * Vis_Schlick(NdotV, NdotL, roughness) * NdotL) * nextPayload.color;

    }
    else{
        if(info.uvOffsetBytes == 0){
           payload.color = mat.baseColor.bgr; 
        }
        else{
            const float2 uv0 = asfloat(attributes.Load2(info.uvOffsetBytes+ii.x*8));
            const float2 uv1 = asfloat(attributes.Load2(info.uvOffsetBytes+ii.y*8));
            const float2 uv2 = asfloat(attributes.Load2(info.uvOffsetBytes+ii.z*8));
            const float2 uv = bary.x * uv0 + bary.y * uv1 + bary.z * uv2;
            payload.color = essisiveTex.SampleLevel(pointSampler, uv, 0).rgb;
        }  
    }
    
}

[shader("miss")]
void Miss(inout Payload payload){
    payload.color = float3(0.0f, 0.0f, 0.0f);
}

