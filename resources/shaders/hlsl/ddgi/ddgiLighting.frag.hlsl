#include "indirectLight.hlsl"

struct Light
{
    float3 direction;
    float  intensity;

    float3 color;
    int    shadowMapIndex;
};

struct UniformBuffer0
{
    Light  lights[4];

    float3 cameraPos;
    int    lightNum;

    float3 probePos0;
    int    padding0;
    float3 probePos1;
    int    padding1;
};


[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UniformBuffer0 ubo0;
};

[[vk::binding(1, 0)]]
cbuffer ub1 : register(b1)
{
    UniformBuffer1 ubo1;
};

[[vk::binding(2, 0)]]Texture2D<float4>   gBufferNormal : register(t0);
[[vk::binding(3, 0)]]Texture2D<float4>   gBufferPosition : register(t1);
[[vk::binding(4, 0)]]Texture2D<float4>   gAlbedo : register(t2);
[[vk::binding(5, 0)]]Texture2D<float4>   gMetallicAndRoughness : register(t3);
[[vk::binding(6, 0)]]Texture2D<float4>   VolumeProbeDatasRadiance  : register(t4);   //[512, 64]
[[vk::binding(7, 0)]]Texture2D<float4>   VolumeProbeDatasDepth  : register(t5);   //[2048, 256]
[[vk::binding(8, 0)]]SamplerState        linearSampler : register(s0);

[[vk::binding(9, 0)]]RaytracingAccelerationStructure Tlas : register(t6);


struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    float4 directLight : SV_Target0;
    float4 indirectLight : SV_Target1;
};

bool RayTracingAnyHit(in RayDesc rayDesc) {
  uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

  RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;

  q.TraceRayInline(Tlas, rayFlags, 0xFF, rayDesc);
  q.Proceed();

  if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
    return true;
  }

  return false;
}

float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function ----------------------------------------------------
float3 F_Schlick(float cosTheta, float metallic)
{
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), float3(1.f, 1.f, 1.f), metallic); // * material.specular
    float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    return F;
}

// Specular BRDF composition --------------------------------------------

float3 BRDF(float3 diffuseColor, float3 lightColor, float3 L, float3 V, float3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
    float3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed
	//float3 lightColor = float3(1.0);

    float3 color = float3(0.0, 0.0, 0.0);

    if (dotNL > 0.0)
    {
        float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
        float3 F = F_Schlick(dotNV, metallic);

        float3 spec = D * F * G / ((4.0 * dotNL * dotNV) + 0.0001);
        float3 diff = (1.f - F) * (1.f - metallic) * diffuseColor / PI;

        color += (spec + diff) * dotNL * lightColor;
    }

    return color;
}

float3 rgb2srgb(float3 color)
{
    color.x = color.x < 0.0031308 ? color.x * 12.92f : pow(color.x, 1.0 / 2.4) * 1.055f - 0.055f;
    color.y = color.y < 0.0031308 ? color.y * 12.92f : pow(color.y, 1.0 / 2.4) * 1.055f - 0.055f;
    color.z = color.z < 0.0031308 ? color.z * 12.92f : pow(color.z, 1.0 / 2.4) * 1.055f - 0.055f;

    return color;
}


PSOutput main(PSInput input)
{
    PSOutput output;
    
    float4 gBufferValue0 = gBufferNormal.Sample(linearSampler, input.texCoord);
    float4 gBufferValue1 = gBufferPosition.Sample(linearSampler, input.texCoord);
    float4 gBufferValue2 = gAlbedo.Sample(linearSampler, input.texCoord);
    float4 gBufferValue3 = gMetallicAndRoughness.Sample(linearSampler, input.texCoord);

    float3 fragNormal = normalize(gBufferValue0.rgb);
    float3 fragPos = gBufferValue1.rgb;
    float2 fragUV = float2(gBufferValue0.a, gBufferValue1.a);
    float4 fragAlbedo = gBufferValue2.rgba;
    float metallic = gBufferValue3.b;
    float roughness = gBufferValue3.g;
    
    float3 directLight = float3(0.f, 0.f, 0.f);

    for (int i = 0; i < ubo0.lightNum; i++)
    {
        RayDesc ray;
        ray.Origin = fragPos;
        ray.Direction = normalize(-ubo0.lights[i].direction);
        ray.TMin = 0.001f;
        ray.TMax = 10000.f;

        bool hasHit = RayTracingAnyHit(ray);
    
        float3 L = normalize(-ubo0.lights[i].direction);
        float3 V = normalize(ubo0.cameraPos.xyz - fragPos);
        float3 R = reflect(-V, fragNormal);

        directLight += (1.f - hasHit) * BRDF(fragAlbedo.rgb, ubo0.lights[i].intensity * ubo0.lights[i].color, L, V, fragNormal, metallic, roughness);
    }

    float3 indirectLight = CalculateIndirectLighting(
        ubo1,
        VolumeProbeDatasRadiance,  
        VolumeProbeDatasDepth, 
        linearSampler,
        ubo0.probePos0,
        ubo0.probePos1,
        fragPos, 
        fragNormal,
        fragAlbedo);
    //indirectLight = indirectLight * fragAlbedo.rgb / PI;

    //float radiance = VolumeProbeDatasRadiance.Sample(linearSampler, float2(0.f, 0.f)).r;

    output.directLight = float4(directLight, 1.f);
    output.indirectLight = float4(indirectLight, 1.f); 
                  
    return output;   
}
