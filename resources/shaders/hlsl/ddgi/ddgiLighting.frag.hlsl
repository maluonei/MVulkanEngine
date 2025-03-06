#include "indirectLight.hlsl"
#include "shading.hlsl"

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
    float4 depthProbeUV : SV_Target2;
    float4 radianceProbeUV : SV_Target3;
    float4 probeRadiance : SV_Target4;
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

        float3 lightColor = ubo0.lights[i].color * ubo0.lights[i].intensity;
        //float3 L = -ubo2.lights[i].direction;
        //float3 V = -ray.Direction;
        //float3 N = output.normal;
        directLight += (1.f - hasHit) * BRDF(fragAlbedo.rgb, lightColor, L, V, fragNormal, metallic, roughness);
        //directLight += (1.f - hasHit) * BRDF(fragAlbedo.rgb, ubo0.lights[i].intensity * ubo0.lights[i].color, L, V, fragNormal, metallic, roughness);
    }      
                   
    IndirectLightingOutput indirectLight = CalculateIndirectLighting(             
        ubo1,   
        VolumeProbeDatasRadiance,     
        VolumeProbeDatasDepth,       
        linearSampler,       
        ubo0.probePos0,   
        ubo0.probePos1,  
        fragPos,   
        fragNormal);

    output.directLight = float4(directLight, 1.f);
    output.indirectLight = float4(indirectLight.radiance * fragAlbedo.rgb / PI, 1.f); 
    output.radianceProbeUV = float4(indirectLight.radianceProbeUV, 1.f);
    output.depthProbeUV = float4(indirectLight.depthProbeUV, 1.f);
    output.probeRadiance = float4(indirectLight.probeRadiance, 1.f);
                   
    return output;   
}  
   