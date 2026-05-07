// This file is gonna be overcommented so the evil and hard BRDF logic sticks in my head.


static const float PI = 3.14159265f;

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
    float a = pow(roughness, 2);
    
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);
    
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;
    
    float3 UpVector = abs(normal.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, normal));
    float3 TangentY = cross(normal, TangentX);
    
    return TangentX * H.x + TangentY * H.y + normal * H.z;
}

// Function provided by Brian Karis from Epic Games.
float2 IntegrateBRDF(float roughness, float NoV)
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

float3 ApproximateSpecularIBL(float3 pbrSpecularColour, float materialRoughness, float3 worldNormal, float3 viewDir)
{
    float NoV = saturate(dot(worldNormal, viewDir));
    float3 R = 2 * dot(viewDir, worldNormal) * worldNormal - viewDir;
    
    float3 PrefilteredColour = float3(0, 0, 0);
    float2 EnvBRDF = IntegrateBRDF(materialRoughness, NoV);
    
    return PrefilteredColour * (pbrSpecularColour * EnvBRDF.x + EnvBRDF.y);
}