#include "Common.hlsl"

[shader("miss")] 
void Miss(inout HitInfo payload
                           : SV_RayPayload) {
    
    float3 rayDir = WorldRayOrigin();
    float3 rayDimension = DispatchRaysDimensions();
    
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    
    float3 blue = float3(0, 1, 0);
    float3 red = float3(1, 0, 0);
    float3 purple = float3(1, 0, 1);
    
    float ramp =  saturate(launchIndex.y / dims.y);
    float3 colour = lerp(red, blue, ramp);
    
    ramp =  saturate(launchIndex.x / dims.x);
    float3 colour2 = lerp(colour, purple, ramp);
    
    
    payload.colorAndDistance += float4(colour2, 1);
}