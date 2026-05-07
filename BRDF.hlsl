// This file is gonna be overcommented so the evil and hard BRDF logic sticks in my head.


float2 Hammersly(int index, int numSamples) // TODO
{
    return float2(0, 0);
}


// NoV - normal dot view. The dot product between the view dir and the surface normal.
// NoL - normal dot light. the dot product between the product between the surface normal and light direction.
float G_Smith(float rougness, float NoV, float NoL)
{
    return 0.0f;
}

float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 normal)
{
    return float3(0,0,0);
}

// Function provided by Brian Karis from Epic Games.
float3 IntegrateBRDF(float roughness, float NoV)
{
    float3 V;
    V.x = sqrt(1.0f - NoV * NoV);
    V.y = 0;
    V.z = NoV;
    
    float A = 0;
    float B = 0;
    
    const uint NumSamples = 1024;
    for (uint i = 0; i < NumSamples; i++)
    {
        float2 Xi = Hammersly(i, NumSamples);
        float3 H = ImportanceSampleGGX(Xi, roughness, N);
        float4 L = 2 * dot(V, H) * H - V;
        
        float NoL = saturate(L.z);
        float NoH = saturate(H.z);
        float VoH = saturate(dot(H, V));
        
        if (NoL > 0)
        {
            float G = G_Smith(roughness, NoV, NoL);
            
            float G_Vis = G * VoH / (NoH * NoV);
            float Fc = pow(1 - VoH, 5);
            
            A += (1 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    return float2(A, B) / NumSamples;
}

float3 SpecularIBL(float3 pbrSpecularColour, float materialRoughness, float3 worldNormal, float3 viewDir)
{
    return pbrSpecularColour;
}