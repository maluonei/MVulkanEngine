#include "indirectLight.hlsli"
#include "shading.hlsli"

[[vk::binding(0, 0)]]
cbuffer lightBuffer : register(b0)
{
    LightBuffer lightBuffer;
}

[[vk::binding(1, 0)]]
cbuffer cameraBuffer : register(b1)
{
    MCameraBuffer cameraBuffer;
}

//[[vk::binding(2, 0)]]
//cbuffer screenBuffer : register(b2)
//{
//    MScreenBuffer screenBuffer;
//}

[[vk::binding(3, 0)]]
cbuffer ddgiBuffer : register(b3)
{
    DDGIBuffer ddgiBuffer;
};

[[vk::binding(4, 0)]] StructuredBuffer<DDGIProbe> probes : register(t7);
[[vk::binding(5, 0)]]Texture2D<float4>   gBufferNormal : register(t0);
[[vk::binding(6, 0)]]Texture2D<float4>   gBufferPosition : register(t1);
[[vk::binding(7, 0)]]Texture2D<float4>   gAlbedo : register(t2);
[[vk::binding(8, 0)]]Texture2D<float4>   gMetallicAndRoughness : register(t3);
[[vk::binding(9, 0)]]Texture2D<float4>   VolumeProbeDatasRadiance  : register(t4);   //[512, 64]
[[vk::binding(10, 0)]]Texture2D<float4>   VolumeProbeDatasDepth  : register(t5);   //[2048, 256]
[[vk::binding(11, 0)]]SamplerState        linearSampler : register(s0);

[[vk::binding(12, 0)]]RaytracingAccelerationStructure Tlas : register(t6);

 
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
 
PSOutput main(PSInput input)
{
    PSOutput output;
    output.directLight = float4(0.f, 0.f, 0.f, 1.f);
    output.indirectLight = float4(0.f, 0.f, 0.f, 1.f);

    float4 gBufferValue0 = gBufferNormal.Sample(linearSampler, input.texCoord);
    if (gBufferValue0.w == 0.f) return output;
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

    for (int i = 0; i < lightBuffer.lightNum; i++)  
    {  
        RayDesc ray;
        ray.Origin = fragPos + 1e-3f * fragNormal;
        ray.Direction = normalize(-lightBuffer.lights[i].direction); 
        ray.TMin = 0.001f;   
        ray.TMax = 10000.f;     
  
        bool hasHit = RayTracingAnyHit(ray);

        float3 L = normalize(-lightBuffer.lights[i].direction);
        float3 V = normalize(cameraBuffer.cameraPos.xyz - fragPos);
        float3 R = reflect(-V, fragNormal);

        float3 lightColor = lightBuffer.lights[i].color * lightBuffer.lights[i].intensity;
        directLight += (1.f - hasHit) * BRDF(fragAlbedo.rgb, lightColor, L, V, fragNormal, metallic, roughness);
        //directLight += (1.f - hasHit) * BRDF(fragAlbedo.rgb, lightBuffer.lights[i].intensity * lightBuffer.lights[i].color, L, V, fragNormal, metallic, roughness);
    }

    IndirectLightingOutput indirectLight = CalculateIndirectLighting(
        ddgiBuffer,
        probes,
        VolumeProbeDatasRadiance,
        VolumeProbeDatasDepth,
        linearSampler,
        ddgiBuffer.probePos0,
        ddgiBuffer.probePos1,  
        fragPos,   
        fragNormal);

    output.directLight = float4(directLight, 1.f);
    output.indirectLight = float4(indirectLight.radiance * fragAlbedo.rgb / PI, 1.f); 

    return output;   
}  
   