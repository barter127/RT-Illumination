#include "Common.hlsl"

struct STriVertex
{ // IMPORTANT - the c++ version of this is 'Vertex' found in the common.h file
    float3 vertex;
    float4 normal;
    float2 texcoord;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);

cbuffer LightParams : register(b0)
{
    float4 lightPosition;
    float4 lightAmbientColour;
    float4 lightDiffuseColour;
    float4 lightSpecularColour;
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

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = 3 * PrimitiveIndex();
    
    float3 vertexNormals[3];
    
    vertexNormals[0] = BTriVertex[indices[vertID + 0]].normal.xyz;
    vertexNormals[1] = BTriVertex[indices[vertID + 1]].normal.xyz;
    vertexNormals[2] = BTriVertex[indices[vertID + 2]].normal.xyz;
    
    float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
    float3 worldNormal = normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
    
    float3 lightDir = float3(0, 1, 0);
    float3 viewDir = WorldRayOrigin();
    
    float diff = saturate(dot(worldNormal, lightDir));
    float4 diffuseCalc = diff * lightDiffuseColour;
    
    float3 halfwayVector = normalize(lightDir + viewDir);
    float4 specularCalc = pow(saturate(dot(worldNormal, halfwayVector)), 28);
    
    float4 finalCol = lightAmbientColour + diffuseCalc + specularCalc;
    
    payload.colorAndDistance = float4(finalCol);
}

[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = 3 * PrimitiveIndex();
    
    float3 vertexNormals[3];
    
    vertexNormals[0] = BTriVertex[indices[vertID + 0]].normal.xyz;
    vertexNormals[1] = BTriVertex[indices[vertID + 1]].normal.xyz;
    vertexNormals[2] = BTriVertex[indices[vertID + 2]].normal.xyz;
    
    float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
    float3 worldNormal = normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
    
    float diff = max(dot(worldNormal, WorldRayDirection()), 0.0f);
    float4 diffuseCalc = diff * lightDiffuseColour;
    float4 finalCol = lightAmbientColour + diffuseCalc;
    
    payload.colorAndDistance = float4(finalCol);
}