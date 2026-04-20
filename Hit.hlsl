#include "Common.hlsl"
#include "Random.hlsl"

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
    float shininess;
    float attenuationRadius;
    float2 padding;
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

float3 SampleSphere(float3 center, float radius)
{
    float3 dir = RandomDirection(state);
    return center + dir * radius;
}

bool TraceShadowRay()
{
     // Fire Shadow Ray.
    RayDesc ray;
    ray.Origin = HitWorldPosition();
    ray.Direction = normalize(lightPosition.xyz - ray.Origin);
    
    ray.TMin = 0.01;
    ray.TMax = length(lightPosition.xyz - ray.Origin); // Ensure ray isn't too large.
    
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

float AccumulateSoftShadowHits(int numRays, float3 lightCentre, float radius, float3 surfaceNormal)
{
    int hitCount = 0;
    
    for (int i = 0; i < numRays; i++)
    {
        // Fire Soft Shadow Ray.
        RayDesc ray;
        ray.Origin = HitWorldPosition();
        
        float3 targetPos = SampleSphere(lightCentre, radius);
        float3 direction = normalize(targetPos - ray.Origin);

        
        ray.Direction = direction;
    
        ray.TMin = 0.01;
        ray.TMax = length(targetPos - ray.Origin); // Ensure ray isn't too large.
    
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
    
    return clamp(1 - ((float) hitCount / (float)numRays), 0.4, 1);
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
        float3 lightDir = normalize((float3) lightPosition - hitPos);
        float3 viewDir = normalize(WorldRayOrigin() - hitPos);

        float dist = length((float3) lightPosition - hitPos);
        attenuation = saturate(1.0f - pow(dist / attenuationRadius, 2.0f));
        
        // Diffuse.
        float diff = saturate(dot(worldNormal, lightDir));
        float4 diffuseCalc = diff * lightDiffuseColour;
    
        // Specular.
        float3 halfwayVector = normalize(lightDir + viewDir);
        float spec = pow(saturate(dot(worldNormal, halfwayVector)), shininess /*ToDo add Specular Power*/);
        float4 specularCalc = spec * lightSpecularColour;
        
        finalCol += (diffuseCalc + specularCalc);
    }
    else
    {
        softShadowMultiplier = AccumulateSoftShadowHits(64, (float3)lightPosition, attenuationRadius, worldNormal);
    }
    
    
    payload.colorAndDistance = float4(finalCol * softShadowMultiplier);
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
    
    float4 ambientCalc = lightAmbientColour;
    
    float4 finalCol = ambientCalc;
    float attenuation = 1.0f;
    float softShadowMultiplier = 1.0f;
    
    float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
    float3 worldNormal = normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
    
    if (!ShadowRayHit)
    {
    
        float3 hitPos = HitWorldPosition();
        float3 lightDir = normalize((float3) lightPosition - hitPos);
        float3 viewDir = normalize(WorldRayOrigin() - hitPos);

        float dist = length((float3) lightPosition - hitPos);
        attenuation = saturate(1.0f - pow(dist / attenuationRadius, 2.0f));
        
        // Diffuse.
        float diff = saturate(dot(worldNormal, lightDir));
        float4 diffuseCalc = diff * lightDiffuseColour;
    
        // Specular.
        float3 halfwayVector = normalize(lightDir + viewDir);
        float spec = pow(saturate(dot(worldNormal, halfwayVector)), shininess /*ToDo add Specular Power*/);
        float4 specularCalc = spec * lightSpecularColour;
        
        finalCol += (diffuseCalc + specularCalc);
    }
    else
    {
        softShadowMultiplier = AccumulateSoftShadowHits(64, (float3) lightPosition, attenuationRadius, worldNormal);
    }
    
    
    payload.colorAndDistance = float4(finalCol * softShadowMultiplier);
}