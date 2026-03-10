#include "Common.hlsl"

struct STriVertex
{ // IMPORTANT - the c++ version of this is 'Vertex' found in the common.h file
    float3 vertex;
    float4 colour;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);

[shader("closesthit")]void ClosestHit(inout HitInfo payload,
                                       Attributes attrib)
{
    float3 barycentrics =
      float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint vertId = 3 * PrimitiveIndex();
    // vertId = the first index, vertId + 1 = the second, vertId + 2 = the third
    // e.g. BTriVertex[indices[vertId + 0]]
  
    float3 colourOut = BTriVertex[indices[vertId + 0]].colour * barycentrics.x + 
    BTriVertex[indices[vertId + 1]].colour * barycentrics.y + 
    BTriVertex[indices[vertId + 2]].colour * barycentrics.z;
    
    payload.colorAndDistance = float4(colourOut, RayTCurrent());

}
