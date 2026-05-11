#include "Common.hlsl"

static const int DirectionalLightType = 0;
static const int PointLightType = 1;

struct LightData
{
    float4 position;
    float4 ambientColour;
    float4 diffuseColour;
    float4 specularColour;
    float shininess;
    float attenuationRadius;
    int type;
    int padding;
};

float AccumulateSoftShadowHits(int numRays, float3 lightCentre, float radius, float3 surfaceNormal, in uint recursionDepth);

float4 PointLight(LightData lightData, int lightCount,float3 worldNormal ,int shadowSampleCount, in int recursionDepth)
{
    float3 hitPos = HitWorldPosition();
    float3 lightDir = normalize((float3) lightData.position - hitPos);
    float3 viewDir = normalize(WorldRayOrigin() - hitPos);

    float dist = length((float3) lightData.position - hitPos);
    float attenuation = saturate(1.0f - pow(dist / lightData.attenuationRadius, 2.0f));
        
    // Diffuse.
    float diff = saturate(dot(worldNormal, lightDir));
    float4 diffuseCalc = diff * lightData.diffuseColour;
    
    // Specular.
    float3 halfwayVector = normalize(lightDir + viewDir);
    float spec = pow(saturate(dot(worldNormal, halfwayVector)), lightData.shininess);
    float4 specularCalc = spec * lightData.specularColour;
        
    float softShadowMultiplier = AccumulateSoftShadowHits(shadowSampleCount, (float3) lightData.position, lightData.attenuationRadius, worldNormal, recursionDepth);
        
    return (lightData.ambientColour + diffuseCalc + specularCalc) * softShadowMultiplier * attenuation;
}

float4 DirectionalLight(LightData lightData, int lightCount, float3 worldNormal, int shadowSampleCount, in int recursionDepth)
{
    float3 hitPos = HitWorldPosition();
    float3 lightDir = normalize((float3) lightData.position);
    float3 viewDir = normalize(WorldRayOrigin() - hitPos);

    float dist = length((float3) lightData.position - hitPos);
        
    // Diffuse.
    float diff = saturate(dot(worldNormal, lightDir));
    float4 diffuseCalc = diff * lightData.diffuseColour;
    
    // Specular.
    float3 halfwayVector = normalize(lightDir + viewDir);
    float spec = pow(saturate(dot(worldNormal, halfwayVector)), lightData.shininess);
    float4 specularCalc = spec * lightData.specularColour;
        
    float softShadowMultiplier = AccumulateSoftShadowHits(shadowSampleCount, (float3) lightData.position, lightData.attenuationRadius, worldNormal, recursionDepth);
        
    return (lightData.ambientColour + diffuseCalc + specularCalc) * softShadowMultiplier;
}