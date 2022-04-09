struct MainFrameConstants{
    matrix model;
    matrix view;
    matrix proj;
    float4 cameraPosition;
    float  fov;
};

struct RayTraceMeshInfo{
    uint  indexOffsetBytes;
    uint  positionOffsetBytes;
    uint  normalOffsetBytes;
    uint  tangentOffsetBytes;
    uint  uvOffsetBytes;
};

struct Payload{
    float3 color;
    uint   rayType;
};

SamplerState pointSampler : register(s2);
SamplerState anisoSampler : register(s3);

RWTexture2D<float4> renderTarget : register(u0, space1);
ConstantBuffer<MainFrameConstants> mainConst : register(b0, space1);

RaytracingAccelerationStructure    scene    : register(t0, space1);
StructuredBuffer<RayTraceMeshInfo> meshInfo : register(t1, space1);

ByteAddressBuffer indices     : register(t2, space1);
ByteAddressBuffer attributes  : register(t3, space1);

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

    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    const uint indexOffset = info.indexOffsetBytes;
    const uint posOffset   = info.positionOffsetBytes;   
    const uint3 ii = indices.Load3(indexOffset+PrimitiveIndex()*12);

    if(payload.rayType == 0){

        const float3 normal0 = asfloat(attributes.Load3(info.normalOffsetBytes+ii.x*12));
        const float3 normal1 = asfloat(attributes.Load3(info.normalOffsetBytes+ii.y*12));
        const float3 normal2 = asfloat(attributes.Load3(info.normalOffsetBytes+ii.z*12));

        float3 rayDirection = WorldRayDirection();
        float3 normal  = bary.x * normal0 + bary.y * normal1 + bary.z * normal2;
        normal = mul(normal, (float3x3)WorldToObject3x4());


        if(dot(rayDirection, normal) > 0.0f){
            normal = -normal;
        }

        RayDesc rayDesc = {
            WorldRayOrigin() + WorldRayDirection() * RayTCurrent(),
            0.1f, reflect(rayDirection, normal),
            asfloat(0x7F7FFFFF)
        };

        Payload nextPayload;
        nextPayload.rayType = 1;
        TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, rayDesc, nextPayload);
        payload.color = nextPayload.color;

    }
    else{
        
        if(info.uvOffsetBytes == 0){
           payload.color = float3(1.0f, 1.0f, 1.0f); 
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

