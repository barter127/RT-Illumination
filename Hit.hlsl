#include "Common.hlsl"
#include "Random.hlsl"

#define MAX_RAY_RECURSION_DEPTH 8

struct STriVertex
{ // IMPORTANT - the c++ version of this is 'Vertex' found in the common.h file
    float3 vertex;
    float4 normal;
    float2 texcoord;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);
Texture2D<float4> g_texture : register(t3);
SamplerState g_sampler : register(s0);

cbuffer LightParams : register(b0)
{
    float4 lightPosition;
    float4 lightAmbientColour;
    float4 lightDiffuseColour;
    float4 lightSpecularColour;
    float shininess;
    float attenuationRadius;
    float2 paddingLight;
};

cbuffer DebugParams : register(b1)
{
    int shadowSampleCount;
    float materialAlbedo;
    float materialRoughness;
    float materialMetalness;
};

float3 HitAttributeFloat3(float3 vertexAttribute[3], Attributes attrib)
{
    float3 barycentrics = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float3 Point = vertexAttribute[0] * barycentrics.x + vertexAttribute[1] * barycentrics.y + vertexAttribute[2] * barycentrics.z;
    
    return Point;
}

float2 HitAttributeFloat2(float2 vertexAttribute[3], Attributes attrib)
{
    float3 barycentrics = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float2 Point = vertexAttribute[0] * barycentrics.x + vertexAttribute[1] * barycentrics.y + vertexAttribute[2] * barycentrics.z;
    
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

float AccumulateSoftShadowHits(int numRays, float3 lightCentre, float radius, float3 surfaceNormal, in uint recursionDepth)
{
    if (recursionDepth > MAX_RAY_RECURSION_DEPTH)
        return 1.0f;
    
    recursionDepth++;
    
    int hitCount = 0;
    
    for (int i = 0; i < numRays; i++)
    {
        // Fire Soft Shadow Ray.
        RayDesc ray;
        ray.Origin = HitWorldPosition();
        
        // Get a random point within the sphere.
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
    
    return 1.0 - ((float) hitCount / (float)numRays);
}

float4 TraceRadianceRay(in RayDesc ray, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return float4(0, 0, 0, 0);
    }
    currentRayRecursionDepth++;
    
    // Fire Reflection Ray.
    RayDesc reflectRay;
    reflectRay.Origin = ray.Origin;
    reflectRay.Direction = ray.Direction;
    
    reflectRay.TMin = 0.01;
    reflectRay.TMax = 100000; // Ensure ray isn't too large.
    
        // Init payload.
    HitInfo reflectPayload;
    reflectPayload.colorAndDistance = float4(0, 0, 0, 0);
    reflectPayload.recursionDepth = ++currentRayRecursionDepth;
    
    TraceRay(
        SceneBVH, // AS
        RAY_FLAG_NONE,
        0xFF, // Instance mask.
        0, // Hit shader offset.
        0, // Geometry Stide.
        0, // Index of miss shader.
        reflectRay, // Ray info.
        reflectPayload); // Payload.   
    
    return reflectPayload.colorAndDistance;
}

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = 3 * PrimitiveIndex();
    
    float2 vertexUV[3];
    vertexUV[0] = BTriVertex[indices[vertID + 0]].texcoord.xy;
    vertexUV[1] = BTriVertex[indices[vertID + 1]].texcoord.xy;
    vertexUV[2] = BTriVertex[indices[vertID + 2]].texcoord.xy;
    
    vertexUV[0].y *= -1;
    vertexUV[1].y *= -1;
    vertexUV[2].y *= -1;
    
    float3 vertexNormals[3];
    vertexNormals[0] = BTriVertex[indices[vertID + 0]].normal.xyz;
    vertexNormals[1] = BTriVertex[indices[vertID + 1]].normal.xyz;
    vertexNormals[2] = BTriVertex[indices[vertID + 2]].normal.xyz;
    
    float2 triTexCoord = HitAttributeFloat2(vertexUV, attrib);
    
    float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
    float3 worldNormal = normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
    float4 ambientCalc = lightAmbientColour;
    
    float4 finalCol = ambientCalc;
    float attenuation = 1.0f;
    
    
    float softShadowMultiplier = AccumulateSoftShadowHits(shadowSampleCount, (float3) lightPosition, attenuationRadius, worldNormal, payload.recursionDepth);

    
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
    float spec = pow(saturate(dot(worldNormal, halfwayVector)), shininess);
    float4 specularCalc = spec * lightSpecularColour;
      
    
    // Calculate and apply reflection colour. TODO: Add some sort of value to tweak it. Maybe I could sample textures later too :D
    RayDesc ray;
    ray.Origin = HitWorldPosition();
    ray.Direction = reflect(WorldRayDirection(), worldNormal);
    float4 reflectionColour = TraceRadianceRay(ray, payload.recursionDepth);
    
    // === Accumulate all light data.
    
    // Multiplying shadows here instead of at the end looks nicer as it keeps the ambient value.
    finalCol += (diffuseCalc + specularCalc) * softShadowMultiplier;
    finalCol += reflectionColour;
    finalCol *= attenuation;
    
    
    
    //payload.colorAndDistance = finalCol ;
    payload.colorAndDistance = finalCol + g_texture.SampleLevel(g_sampler, triTexCoord, 0);
}

[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = 3 * PrimitiveIndex();
    
    // I need to make this a function.
    float2 vertexUV[3];
    vertexUV[0] = BTriVertex[indices[vertID + 0]].texcoord.xy;
    vertexUV[1] = BTriVertex[indices[vertID + 1]].texcoord.xy;
    vertexUV[2] = BTriVertex[indices[vertID + 2]].texcoord.xy;
    
    vertexUV[0].y *= -1;
    vertexUV[1].y *= -1;
    vertexUV[2].y *= -1;
    
    float3 vertexNormals[3];
    vertexNormals[0] = BTriVertex[indices[vertID + 0]].normal.xyz;
    vertexNormals[1] = BTriVertex[indices[vertID + 1]].normal.xyz;
    vertexNormals[2] = BTriVertex[indices[vertID + 2]].normal.xyz;
    
    float4 ambientCalc = lightAmbientColour;
    
    float4 finalCol = ambientCalc;
    float attenuation = 1.0f;
    
    float2 triTexCoord = HitAttributeFloat2(vertexUV, attrib);
    
    float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
    float3 worldNormal = normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
    
    float softShadowMultiplier = AccumulateSoftShadowHits(shadowSampleCount, (float3) lightPosition, attenuationRadius, worldNormal, payload.recursionDepth);
    
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
    float spec = pow(saturate(dot(worldNormal, halfwayVector)), shininess);
    float4 specularCalc = spec * lightSpecularColour;
        
    finalCol += (diffuseCalc + specularCalc) * softShadowMultiplier;
    finalCol *= attenuation;
    
    payload.colorAndDistance = finalCol + g_texture.SampleLevel(g_sampler, triTexCoord, 0);
}