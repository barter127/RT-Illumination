// This file is gonna be overcommented so the evil and hard BRDF logic sticks in my head.
// Most of it came from here but I wanted to play around with it.
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf


static const float PI = 3.14159265f;

float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 normal);
float3 G_Smith(float rougness, float NoV, float NoL);
float2 Hammersley(int bits, int numSamples);



// Almost identical to: https://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html.
// The hammersly function is a way to sample from a hemisphere using importance sampling it.
// This is very similar to how I calculated lighting and would be a better approach to randomising the point light ray samples (if I had time).
float2 Hammersley(int bits, int numSamples)
{
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
    bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
    
    float reversedBits = float(bits) * 2.3283064365386963e-10f;
    
    return float2(float(bits) / float(numSamples), reversedBits);
}

// In Unreal's code this func is used to sample the precomputed environment map.
// It uses importance sampling to compute microfacet distrubtution based on viewing angle.
// My implementation uses raytraces for each check so it's very expensive.
float3 PrefilterEnvMap(float Roughness, float3 R)
{
    float3 N = R;
    float3 V = R;
    float3 PrefilteredColor = 0;
    float TotalWeight = 0.0f;
    const uint NumSamples = 1024;
    for (uint i = 0; i < NumSamples; i++)
    {
        float2 Xi = Hammersley(i, NumSamples);
        float3 H = ImportanceSampleGGX(Xi, Roughness, N);
        float3 L = 2 * dot(V, H) * H - V;
        float NoL = saturate(dot(N, L));
        if (NoL > 0)
        {
            // Replace this with a ray trace because I do not have an environment map.
            PrefilteredColor += float4(0,0,0,0);
            TotalWeight += NoL;
        }
    }
    return PrefilteredColor / TotalWeight;
}

// NoV - normal dot view. The dot product between the view dir and the surface normal.
// NoL - normal dot light. the dot product between the product between the surface normal and light direction.
// Essentially the letter o between two letters just represents the dot in dot product.

// 
float G_Smith(float rougness, float NoV, float NoL)
{
    return 0.0f;
}


// Hammersly generates 2D sample points but GGX turns those points into directions.
// Xi will always be an input from a hammersly function.
// Used to account for microfacets distrbuting reflected light at different viewing angles.
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

// This is the BRDF function.
// It's the same function discussed in the week 6 slides and like everything in this file is used to account for mircofacet distribution.
// A rough surface with scatter light all over the place and a smooth surface will gently offset the reflection rays.
// It was also desribed by SimonDev as the equation all graphic programmers are trying to solve but I cannot find the exact video.
float2 IntegrateBRDF(float roughness, float NoV, float3 Normal)
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
        float2 Xi = Hammersley(i, NumSamples);
        float3 H = ImportanceSampleGGX(Xi, roughness, Normal);
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
    float2 EnvBRDF = IntegrateBRDF(materialRoughness, NoV, worldNormal);
    
    return PrefilteredColour * (pbrSpecularColour * EnvBRDF.x + EnvBRDF.y);
}