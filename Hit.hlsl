#include "Common.hlsl"

struct STriVertex
{ // IMPORTANT - the c++ version of this is 'Vertex' found in the common.h file
    float3 vertex;
    float4 normal;
    float2 texcoord;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);

RaytracingAccelerationStructure SceneBVH : register(t2);

cbuffer LightParams : register(b0)
{
    float4 lightPosition;
    float4 lightAmbientColour;
    float4 lightDiffuseColour;
    float4 lightSpecularColour;
    float attenuationRadius;
};

float3 HitAttributeFloat3(float3 vertexAttribute[3], Attributes attrib)
{
    float3 barycentrics = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float3 Point = vertexAttribute[0] * barycentrics.x + vertexAttribute[1] * barycentrics.y + vertexAttribute[2] * barycentrics.z;
    
    return Point;
}

float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float nrand(float2 uv)
{
    // Stolen from stackoverflow.
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

bool TraceShadowRay()
{
     // Fire Shadow Ray.
    RayDesc ray;
    ray.Origin = HitWorldPosition();
    ray.Direction = normalize(lightPosition.xyz - ray.Origin);
    
    ray.TMin = 0.01;
    ray.TMax = length(ray.Direction); // Ensure ray isn't too large.
    
    // Init payload.
    ShadowHitInfo shadowPayload;
    shadowPayload.isHit = false;
    
    TraceRay(
    SceneBVH, // AS
    RAY_FLAG_NONE,
    0xFF, // Instance mask.
    1, // Hit shader offset.
    0, // Geometry Stide.
    1, // Index of miss shader.
    ray, // Ray info.
    shadowPayload); // Payload.   
    
    return shadowPayload.isHit;
}

float CalculateSoftShadow(float3 shadowPoint, float3 surfaceNormal, int sampleCount)
{
    int hitCount = 0;
    
    float3 direction = normalize(lightPosition.xyz - shadowPoint);
    for (int i = 0; i < sampleCount; i++)
    {
        float3 offsetPoint = shadowPoint + (0.005f * i) * surfaceNormal;
        float3 offsetDir = direction + nrand(float2(DispatchRaysIndex().x, DispatchRaysIndex().y)) * surfaceNormal;
        
        // Fire Shadow Ray.
        RayDesc ray;
        ray.Origin = offsetPoint;
        ray.Direction = direction;
    
        ray.TMin = 0.001;
        ray.TMax = length((float3)lightPosition - shadowPoint);
;
    
        // Init payload.
        ShadowHitInfo shadowPayload;
        shadowPayload.isHit = false;
    
        TraceRay(
        SceneBVH, // AS
        RAY_FLAG_NONE,
        0xFF, // Instance mask.
        1, // Hit shader offset.
        0, // Geometry Stide.
        1, // Index of miss shader.
        ray, // Ray info.
        shadowPayload); // Payload.   
    
        if (shadowPayload.isHit)
        {
            hitCount++;
        }
    }

    return saturate(1 - ((float) hitCount / (float)sampleCount));

}

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = 3 * PrimitiveIndex();
    
    float3 vertexNormals[3];
    
    vertexNormals[0] = BTriVertex[indices[vertID + 0]].normal.xyz;
    vertexNormals[1] = BTriVertex[indices[vertID + 1]].normal.xyz;
    vertexNormals[2] = BTriVertex[indices[vertID + 2]].normal.xyz;
    
    bool ShadowRayHit = TraceShadowRay();
    
    float4 ambientCalc = lightAmbientColour;
    
    float4 finalCol = ambientCalc;
    float attenuation = 1.0f;
    float softShadowMultiplier = 1.0f;
    
    float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
    float3 worldNormal = normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
    
    if (!ShadowRayHit)
    {
    
        float3 hitPos = HitWorldPosition();
        float3 lightDir = normalize((float3)lightPosition - hitPos);
        float3 viewDir = normalize(WorldRayOrigin() - hitPos);

        float dist = length((float3) lightPosition - hitPos);
        attenuation = saturate(1.0f - pow(dist / attenuationRadius, 2.0f));
        
        // Diffuse.
        float diff = saturate(dot(worldNormal, lightDir));
        float4 diffuseCalc = diff * lightDiffuseColour;
    
        // Specular.
        float3 halfwayVector = normalize(lightDir + viewDir);
        float4 specularCalc = pow(saturate(dot(worldNormal, halfwayVector)), 28/*ToDo add Specular Power*/);
        
        finalCol += diffuseCalc + specularCalc;
    }
    else
    {
        softShadowMultiplier = CalculateSoftShadow(HitWorldPosition(), worldNormal, 1028);
    }
    
    payload.colorAndDistance = float4(finalCol * attenuation * softShadowMultiplier);
}

[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = 3 * PrimitiveIndex();
    
    float3 vertexNormals[3];
    
    vertexNormals[0] = BTriVertex[indices[vertID + 0]].normal.xyz;
    vertexNormals[1] = BTriVertex[indices[vertID + 1]].normal.xyz;
    vertexNormals[2] = BTriVertex[indices[vertID + 2]].normal.xyz;
    
    bool ShadowRayHit = TraceShadowRay();
    
    float4 ambientCalc = lightAmbientColour * (ShadowRayHit ? 0.3f : 1.0f);
    
    float4 finalCol = ambientCalc;
    
    float softShadowMultiplier = 1.0f;
    
    float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
    float3 worldNormal = normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
    
    if (!ShadowRayHit)
    {
        float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
        float3 worldNormal = normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
    
        float3 lightDir = float3(0, 1, 0);
        float3 viewDir = WorldRayOrigin();
        
        // Diffuse.
        float diff = saturate(dot(worldNormal, lightDir));
        float4 diffuseCalc = diff * lightDiffuseColour;
    
        // Specular.
        float3 halfwayVector = normalize(lightDir + viewDir);
        float4 specularCalc = pow(saturate(dot(worldNormal, halfwayVector)), 28 /*ToDo add Specular Power*/);
        
        
        
        finalCol += diffuseCalc + specularCalc;
    }
    else
    {
        softShadowMultiplier = CalculateSoftShadow(HitWorldPosition(), worldNormal, 64);
    }
    
    payload.colorAndDistance = float4(finalCol * softShadowMultiplier);
}