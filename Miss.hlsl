#include "Common.hlsl"

[shader("miss")] void Miss(inout HitInfo payload
                           : SV_RayPayload) {
    
    float3 rayDir = WorldRayOrigin();
    
    float compressed = ((rayDir.y + 1 * 0.5f));
    payload.colorAndDistance = float4(compressed, 0.0f, 0.0f, 1);
}