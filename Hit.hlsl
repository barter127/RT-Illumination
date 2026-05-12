// #include "Common.hlsl"
#include "Random.hlsl"
#include "Light.hlsl"
#include "BRDF.hlsl"

#define MAX_RAY_RECURSION_DEPTH 8

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);

Texture2D<float4> g_bTexture : register(t3);
Texture2D<float4> g_bMetalMap : register(t4);
Texture2D<float4> g_bNormal : register(t5);
Texture2D<float4> g_dTexture : register(t6);
Texture2D<float4> g_dMetalMap : register(t7);
Texture2D<float4> g_dNormal : register(t8);

SamplerState g_samplerPoint : register(s0);
SamplerState g_samplerLinear : register(s1);

static const int lightCount = 4;
cbuffer LightBuffer : register(b0)
{
    LightData lightArray[lightCount];
};

cbuffer DebugParams : register(b1)
{
    int shadowSampleCount;
    float materialAlbedo; // Unimplemented.
    float materialRoughness;
    float materialMetalness; // Unimplemented
    
    bool usePointSample;
    float3 padding;
};

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
        // if nothing has been hit after this many rays exit search and assume it is unshadowed.
        const int ShadowedCutoffCount = 4;
        if (numRays == ShadowedCutoffCount) 
        {
            if (hitCount <= 0)
                return 1.0f;
                
            else if (hitCount == ShadowedCutoffCount)
                return 0.0f;
        }
        
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


// Used to render bunnies. Fully lit with perfect reflections. (no roughness).
[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{   
    float2 triTexCoord = ComputeWorldUVs(BTriVertex, indices, attrib);
    
    float3 worldNormal = ComputeWorldNormals(BTriVertex, indices, attrib);
    
    float4 finalCol = float4(0, 0, 0, 0);
    for (int i = 0; i < lightCount; i++)
    {
        if (lightArray[i].type == DirectionalLightType)
            finalCol += DirectionalLight(lightArray[i], lightCount, worldNormal, shadowSampleCount, payload.recursionDepth);
        
        else if (lightArray[i].type == PointLightType)
            finalCol += PointLight(lightArray[i], lightCount, worldNormal, shadowSampleCount, payload.recursionDepth);
    }
    
    // Calculate and apply reflection colour. TODO: Add some sort of value to tweak it. Maybe I could sample textures later too :D
    RayDesc ray;
    ray.Origin = HitWorldPosition();
    ray.Direction = reflect(WorldRayDirection(), worldNormal);
    float4 reflectionColour = TraceRadianceRay(ray, payload.recursionDepth);
    
    // === Accumulate all light data.
    
    finalCol += reflectionColour;
    
    float4 baseColour;
    
    if (usePointSample)
        baseColour = g_bTexture.SampleLevel(g_samplerPoint, triTexCoord, 0);
    else
        baseColour = g_bTexture.SampleLevel(g_samplerLinear, triTexCoord, 0);
    
    payload.colorAndDistance = finalCol * baseColour;
}

// Used to render Dragons. Fully lit with attmpt at PBR reflections using an Environment app see BRDF.hlsl.
[shader("closesthit")]
void DragonClosestHit(inout HitInfo payload, Attributes attrib)
{
    float2 triTexCoord = ComputeWorldUVs(BTriVertex, indices, attrib);
    float3 worldNormal = ComputeWorldNormals(BTriVertex, indices, attrib);
    
    float4 finalCol = float4(0, 0, 0, 0);
    for (int i = 0; i < lightCount; i++)
    {
        if (lightArray[i].type == DirectionalLightType)
            finalCol += DirectionalLight(lightArray[i], lightCount, worldNormal, shadowSampleCount, payload.recursionDepth);
        else if (lightArray[i].type == PointLightType)
            finalCol += PointLight(lightArray[i], lightCount, worldNormal, shadowSampleCount, payload.recursionDepth);
    }
    
    float3 hitPos = HitWorldPosition();
    float3 viewDir = normalize(WorldRayOrigin() - hitPos);
    
    float4 baseColour = float4(0,0,0,1);
    
    if (usePointSample)
        baseColour = g_dTexture.SampleLevel(g_samplerPoint, triTexCoord, 0);
    else
        baseColour = g_dTexture.SampleLevel(g_samplerLinear, triTexCoord, 0);
    
    baseColour.rgb += ApproximateSpecularIBL((float3) lightArray[0].specularColour, materialRoughness,
        worldNormal, viewDir, g_samplerLinear);
    
    float4 sampe = g_dMetalMap.SampleLevel(g_samplerPoint, triTexCoord, 0) + g_dNormal.SampleLevel(g_samplerPoint, triTexCoord, 0);
    
    payload.colorAndDistance = baseColour * finalCol;
}

// Fully lit untextured cube. Mainly to showcase the soft shadowing.
[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
    float2 triTexCoord = ComputeWorldUVs(BTriVertex, indices, attrib);
    float3 worldNormal = ComputeWorldNormals(BTriVertex, indices, attrib);
    
    float4 finalCol = float4(0, 0, 0, 0);
    for (int i = 0; i < lightCount; i++)
    {
        if (lightArray[i].type == DirectionalLightType)
            finalCol += DirectionalLight(lightArray[i], lightCount, worldNormal, shadowSampleCount, payload.recursionDepth);
        
        else if (lightArray[i].type == PointLightType)
            finalCol += PointLight(lightArray[i], lightCount, worldNormal, shadowSampleCount, payload.recursionDepth);
    }
    
    payload.colorAndDistance = finalCol;
}

[shader("anyhit")]
void ArmadilloAnyHit(inout HitInfo payload, Attributes attrib)
{
    payload.colorAndDistance = float4(1, 1, 1, 1);
}