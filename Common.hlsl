// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.

#ifndef COMMONTYPES
#define COMMONTYPES
#define MAX_RAY_RECURSION_DEPTH 8

struct HitInfo {
    float4 colorAndDistance;
    uint recursionDepth;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes {
    float2 bary;
};

// Holds flag to see if object is occluded.
struct ShadowHitInfo
{
    bool isHit;
};

struct STriVertex
{ // IMPORTANT - the c++ version of this is 'Vertex' found in the common.h file
    float3 vertex;
    float4 normal;
    float2 texcoord;
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

// Helper function for calculating the hit UV.
float2 ComputeWorldUVs(StructuredBuffer<STriVertex> BTriVertex, StructuredBuffer<int> indices, Attributes attrib)
{
    uint vertID = 3 * PrimitiveIndex();
        
    float2 vertexUV[3];
    vertexUV[0] = BTriVertex[indices[vertID + 0]].texcoord.xy;
    vertexUV[1] = BTriVertex[indices[vertID + 1]].texcoord.xy;
    vertexUV[2] = BTriVertex[indices[vertID + 2]].texcoord.xy;
    
    vertexUV[0].y *= -1;
    vertexUV[1].y *= -1;
    vertexUV[2].y *= -1;
    
    return HitAttributeFloat2(vertexUV, attrib);
}

// Helper function for calculating the world normal.
float3 ComputeWorldNormals(StructuredBuffer<STriVertex> BTriVertex, StructuredBuffer<int> indices, Attributes attrib)
{
    uint vertID = 3 * PrimitiveIndex();
        
    float3 vertexNormals[3];
    vertexNormals[0] = BTriVertex[indices[vertID + 0]].normal.xyz;
    vertexNormals[1] = BTriVertex[indices[vertID + 1]].normal.xyz;
    vertexNormals[2] = BTriVertex[indices[vertID + 2]].normal.xyz;
    
    float3 triNormal = HitAttributeFloat3(vertexNormals, attrib);
    return normalize(mul(triNormal, (float3x3) ObjectToWorld4x3()));
}
#endif