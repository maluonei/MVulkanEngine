struct Light
{
    float4x4 shadowViewProj;

    float3 direction;
    float intensity;

    float3 color;
    int shadowMapIndex;

    float cameraZnear;
    float cameraZfar;
    float padding6;
    float padding7;
};

struct UnifomBuffer0
{
    Light lights[2];

    float3 cameraPos;
    int lightNum;
    
    int ResolutionWidth;
    int ResolutionHeight;
    int Width;
    int Height;
};

[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UnifomBuffer0 ubo0;
}

[[vk::binding(1, 0)]] Texture2D<float4> gBufferNormal : register(t0);
[[vk::binding(2, 0)]] Texture2D<float4> gBufferPosition : register(t1);
[[vk::binding(3, 0)]] Texture2D<float4> gAlbedo : register(t2);
[[vk::binding(4, 0)]] Texture2D<float4> gMetallicAndRoughness : register(t3);
[[vk::binding(5, 0)]] RWTexture2D<float4> OutputTexture : register(t1); // ���������
[[vk::binding(6, 0)]] SamplerState linearSampler : register(s0);

[[vk::binding(7, 0)]] RaytracingAccelerationStructure Tlas;

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
    float3 F0 = lerp(float3(0.04), float3(1.f), metallic); // * material.specular
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

    float3 color = float3(0.0);

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


[numthreads(16, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    float2 texCoord = float2(threadID.x/(float)(Width), threadID.y/(float)(Height));
    
    float4 gBufferValue0 = gBufferNormal.Sample(linearSampler, texCoord);
    float4 gBufferValue1 = gBufferPosition.Sample(linearSampler, texCoord);
    float4 gBufferValue2 = gAlbedo.Sample(linearSampler, texCoord);
    float4 gBufferValue3 = gMetallicAndRoughness.Sample(linearSampler, texCoord);

    float3 fragNormal = normalize(gBufferValue0.rgb);
    float3 fragPos = gBufferValue1.rgb;
    float2 fragUV = float2(gBufferValue0.a, gBufferValue1.a);
    float4 fragAlbedo = gBufferValue2.rgba;
    float metallic = gBufferValue3.b;
    float roughness = gBufferValue3.g;
    
    float3 fragcolor = float3(0.f, 0.f, 0.f);

    for (int i = 0; i < ubo0.lightNum; i++)
    {
        //float ocllusion = PCF(i, fragPos);
        RayDesc ray;
        PathState pathState;
        ray.Origin = fragPos;
        ray.Direction = normalize(-ubo0.lights[i].direction);
        ray.TMin = 0.001f;
        ray.TMax = 1000.f;

        bool hasHit = RayTracingClosestHit(ray, pathState);
        
        float3 L = normalize(-ubo0.lights[i].direction);
        float3 V = normalize(ubo0.cameraPos.xyz - fragPos);
        float3 R = reflect(-V, fragNormal);

        fragcolor += (1.f - hasHit) * BRDF(fragAlbedo.rgb, ubo0.lights[i].intensity * ubo0.lights[i].color, L, V, fragNormal, metallic, roughness);
        fragcolor += fragAlbedo.rgb * 0.04f; //ambient
    }

    OutputTexture[float2(threadID.x, threadID.y)] = float4(fragcolor, 1.f); // ʹ������ֱ��д��
}