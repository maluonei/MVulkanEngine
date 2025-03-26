#include "indirectLight.hlsli"
#include "shading.hlsli"
#include "SDF.hlsli"

//struct TexBuffer
//{
//    int diffuseTextureIdx;
//    int metallicAndRoughnessTextureIdx;
//    int matId;
//    int padding2;
//};
//    
//struct UniformBuffer0
//{
//    TexBuffer texBuffer[512];
//};

struct GeometryInfo {
  int vertexOffset;
  int indexOffset;
  int uvOffset;
  int normalOffset;
  int materialIdx;
};

struct UniformBuffer2
{
    Light lights[4];

    int    lightNum;
    int    frameCount;
    int    padding1;
    float  t;

    float3 cameraPos;
    float  padding2;
    float4 probeRotateQuaternion;
};

struct SDFBuffer{
    float3 aabbMin;
    uint maxRaymarchSteps;
    float3 aabbMax;
    float padding1;
    uint3 sdfResolution;
    float padding2;
};

//[[vk::binding(0, 0)]]
//cbuffer ubo : register(b0)
//{
//    UniformBuffer0 ubo0;
//};

[[vk::binding(0, 0)]]
cbuffer ubo1 : register(b1)
{
    UniformBuffer1 ubo1;
};

[[vk::binding(1, 0)]]
cbuffer ubo2 : register(b2)
{
    UniformBuffer2 ubo2;
};

[[vk::binding(2, 0)]]
cbuffer ubo3 : register(b2)
{
    SDFBuffer ubo3;
};

[[vk::binding(3, 0)]] StructuredBuffer<Probe> probes : register(t5);
[[vk::binding(4, 0)]] Texture2D<float4> VolumeProbeDatasRadiance  : register(t1031);   //[512, 64]
[[vk::binding(5, 0)]] Texture2D<float4> VolumeProbeDatasDepth  : register(t1032);   //[2048, 256]
//[[vk::binding(6, 0)]] Texture2D<float>  shadowMaps[4]  : register(t1033);   //[2048, 256]

[[vk::binding(6, 0)]] RWTexture3D<float> SDFTexture : register(u0);
[[vk::binding(7, 0)]] RWTexture3D<float4> albedoTexture : register(u1);

[[vk::binding(8, 0)]] SamplerState linearSampler : register(s0);

  
struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    float4 position : SV_Target0;
    float4 radiance : SV_Target1;
};

float3 SphericalFibonacci(float index, float numSamples)
{
    const float b = (sqrt(5.f) * 0.5f + 0.5f) - 1.f;
    float phi = 2 * PI * frac(index * b);
    float cosTheta = 1.f - (2.f * index + 1.f) * (1.f / numSamples);
    float sinTheta = sqrt(saturate(1.f - (cosTheta * cosTheta)));

    return float3((cos(phi) * sinTheta), (sin(phi) * sinTheta), cosTheta);
}

float4 QuaternionConjugate(float4 q)
{
    return float4(-q.xyz, q.w);
}

float3 QuaternionRotate(float3 v, float4 q){
    float3 b = q.xyz;
    float b2 = dot(b, b);
    return normalize(v * (q.w * q.w - b2) + b * (dot(v, b) * 2.f) + cross(b, v) * (q.w * 2.f));
}

float3 ShadingDirectionalLight(
    Light light,
    float3 view
){
  bool  vdotl = dot(view, -light.direction) > 0.f;
  float3 h = normalize(-light.direction + view);
  float  nol = max(dot(h, -light.direction), 0.f);

  return float(vdotl) * light.color * light.intensity * nol;
}



PSOutput main(PSInput input)
{
    const int2 FullResolution = int2(ubo1.raysPerProbe, ubo1.probeDim.x * ubo1.probeDim.y * ubo1.probeDim.z);

    int2 idx = int2(FullResolution * input.texCoord);
    int rayIndex = idx.x;
    int probeIndex = idx.y;
    Probe probe = probes[probeIndex];

    PSOutput output;
    output.radiance.w = 0.f;

    MDFStruct mdf;
    mdf.SDFTexture = SDFTexture;
    mdf.albedoTexture = albedoTexture;
    mdf.aabbMin = ubo3.aabbMin;
    mdf.aabbMax = ubo3.aabbMax;
    mdf.gridResolution = ubo3.sdfResolution;

    float3 position = GetProbePosition(ubo1, probe); 
    float3 direction = SphericalFibonacci(rayIndex, ubo1.raysPerProbe);
    direction = QuaternionRotate(direction, QuaternionConjugate(ubo2.probeRotateQuaternion));
    
    float distance = 0.f;

    float3 directLighting = float3(0.f, 0.f, 0.f);
    float3 indirectLighting = float3(0.f, 0.f, 0.f);

    output.position = float4(0.f, 0.f, 0.f, 0.f);
    output.radiance = float4(0.f, 0.f, 0.f, 0.f);

    if(RayMarchSDF(position, direction, mdf, ubo3.maxRaymarchSteps, distance)){
      float3 hitPoint = position + direction * distance;

      float3 posCoord = (hitPoint - mdf.aabbMin) / (mdf.aabbMax - mdf.aabbMin);
      int3 coord = posCoord * mdf.gridResolution;
      float3 albedo = albedoTexture[coord].rgb;

      output.position = float4(hitPoint, 1.f);

      for(int i=0;i<ubo2.lightNum;i++){
        Light light = ubo2.lights[i];
        //Texture2D<float> shadowMap = shadowMaps[i];

        //bool vis = VisibleToLight(shadowMap, linearSampler, hitPoint, light);
        bool vis = VisibleToLight(hitPoint, light, mdf);
        if(vis){
          //float3 brdf = albedo / PI;
          //float3 lighting = ShadingDirectionalLight(light, -direction);

          float3 lightColor = light.color * light.intensity;
          float3 L = -light.direction;
          float3 V = -direction;
          float3 N = normalize(L+V);

          float3 brdf = BRDF(albedo, lightColor, L, V, N, 0.f, 1.f);
          directLighting += brdf;
        }
      } 

      Light light = ubo2.lights[0];
      float3 L = -light.direction;
      float3 V = -direction;
      float3 N = normalize(L+V);

      IndirectLightingOutput indirectOutput = CalculateIndirectLighting(
                  ubo1,
                  probes,
                  VolumeProbeDatasRadiance,  
                  VolumeProbeDatasDepth, 
                  linearSampler, 
                  ubo1.probePos0,
                  ubo1.probePos1,
                  hitPoint, 
                  N); 

      indirectLighting += indirectOutput.radiance * albedo / PI; 
    }

    output.radiance = float4(directLighting + indirectLighting, distance);
    return output;
}
 