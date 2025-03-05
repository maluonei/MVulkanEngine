#include "indirectLight.hlsl"

struct TexBuffer
{
    int diffuseTextureIdx;
    int metallicAndRoughnessTextureIdx;
    int matId;
    int padding2;
};
    
struct UniformBuffer0
{
    TexBuffer texBuffer[512];
};

struct GeometryInfo {
  int vertexOffset;
  int indexOffset;
  int uvOffset;
  int normalOffset;
  int materialIdx;
};

struct Light
{
    float3 direction;
    float intensity;

    float3 color;
    int padding0;
};

struct UniformBuffer2
{
    Light lights[4];

    float3 probePos0;
    int    lightNum;
    float3 probePos1;
    int    frameCount;

    int    raysPerProbe;
    int    padding0;
    int    padding1;
    int    padding2;
};

[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UniformBuffer0 ubo0;
};

[[vk::binding(1, 0)]]
cbuffer ubo1 : register(b1)
{
    UniformBuffer1 ubo1;
};

[[vk::binding(2, 0)]]
cbuffer ubo2 : register(b2)
{
    UniformBuffer2 ubo2;
};

[[vk::binding(3, 0)]] StructuredBuffer<float> VertexBuffer : register(t0);
[[vk::binding(4, 0)]] StructuredBuffer<int> IndexBuffer : register(t1);
[[vk::binding(5, 0)]] StructuredBuffer<float> NormalBuffer : register(t2);
[[vk::binding(6, 0)]] StructuredBuffer<float> UVBuffer : register(t3);
[[vk::binding(7, 0)]] StructuredBuffer<GeometryInfo> instanceOffset : register(t4);
[[vk::binding(8, 0)]] Texture2D<float4> textures[1024] : register(t5);
[[vk::binding(9, 0)]] Texture2D<float4> VolumeProbeDatasRadiance  : register(t1030);   //[512, 64]
[[vk::binding(10, 0)]] Texture2D<float4> VolumeProbeDatasDepth  : register(t1031);   //[2048, 256]

[[vk::binding(11, 0)]] SamplerState linearSampler : register(s0);

[[vk::binding(12, 0)]] RaytracingAccelerationStructure Tlas : register(t1032);

  
struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 albedo : SV_Target2;
    float4 radiance : SV_Target3;
};

//static const float PI = 3.14159265359f;

struct PathState {
  float3 position;
  float3 normal;
  float3 albedo;
  float3 metallicAndRoughness;
  //float3 rayDirection;

  bool outside;
  float t;
  float2 uv;

  uint instanceID;
  uint primitiveID;
};

bool RayTracingAnyHit(in RayDesc rayDesc, out float t) {
  uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

  RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;

  q.TraceRayInline(Tlas, rayFlags, 0xFF, rayDesc);
  q.Proceed();

  if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
    t = q.CommittedRayT();
    return true;
  }

  return false;
}

bool RayTracingClosestHit(inout RayDesc rayDesc, inout PathState pathState) {
  uint rayFlags = RAY_FLAG_FORCE_OPAQUE;

  RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;

  q.TraceRayInline(Tlas, rayFlags, 0xFF, rayDesc);
  q.Proceed();

  //pathState.rayDirection = rayDesc.Direction;

  pathState.t = 10000.f;
  if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
    pathState.t = q.CommittedRayT();

    uint instanceIndex = q.CommittedInstanceID();

    uint ioffset = instanceOffset[instanceIndex].indexOffset;
    uint voffset = instanceOffset[instanceIndex].vertexOffset;

    uint primitiveIndex = q.CommittedPrimitiveIndex();

    pathState.instanceID = instanceIndex;
    pathState.primitiveID = primitiveIndex;

    uint v0Idx = IndexBuffer[ioffset + primitiveIndex * 3];
    uint v1Idx = IndexBuffer[ioffset + primitiveIndex * 3 + 1];
    uint v2Idx = IndexBuffer[ioffset + primitiveIndex * 3 + 2];

    float3 v0 = float3(VertexBuffer[voffset + v0Idx * 3],
                       VertexBuffer[voffset + v0Idx * 3 + 1],
                       VertexBuffer[voffset + v0Idx * 3 + 2]);
    float3 v1 = float3(VertexBuffer[voffset + v1Idx * 3],
                       VertexBuffer[voffset + v1Idx * 3 + 1],
                       VertexBuffer[voffset + v1Idx * 3 + 2]);
    float3 v2 = float3(VertexBuffer[voffset + v2Idx * 3],
                       VertexBuffer[voffset + v2Idx * 3 + 1],
                       VertexBuffer[voffset + v2Idx * 3 + 2]);

    // compute normal
    float3 e0 = v1 - v0;
    float3 e1 = v2 - v0;

    bool outside = true;
    float3 normal = normalize(cross(e0, e1));
    if(dot(rayDesc.Direction, normal) > 0.f){
        normal = -normal;
        outside = false;
    } 

    int noffset = instanceOffset[instanceIndex].normalOffset;

    float2 barycentrics = q.CommittedTriangleBarycentrics();
    float w0 = 1.f - barycentrics.x - barycentrics.y;
    float w1 = barycentrics.x;  
    float w2 = barycentrics.y;

    float3 position = w0 * v0 + w1 * v1 + w2 * v2;

    if (noffset != -1) {
      float3 n0 = float3(NormalBuffer[noffset + v0Idx * 3],
                         NormalBuffer[noffset + v0Idx * 3 + 1],
                         NormalBuffer[noffset + v0Idx * 3 + 2]);
      float3 n1 = float3(NormalBuffer[noffset + v1Idx * 3],
                         NormalBuffer[noffset + v1Idx * 3 + 1],
                         NormalBuffer[noffset + v1Idx * 3 + 2]);
      float3 n2 = float3(NormalBuffer[noffset + v2Idx * 3],
                         NormalBuffer[noffset + v2Idx * 3 + 1],
                         NormalBuffer[noffset + v2Idx * 3 + 2]);

      normal = normalize(w0 * n0 + w1 * n1 + w2 * n2);
      if(dot(rayDesc.Direction, normal) > 0.f){
        normal = -normal;
        outside = false;
      }
    }

    // TODO
    int uvOffset = instanceOffset[instanceIndex].uvOffset;
    float2 texCoords = float2(0.f, 0.f);

    if (uvOffset != -1) {
      float2 uv0 = float2(UVBuffer[uvOffset + v0Idx * 2],
                          UVBuffer[uvOffset + v0Idx * 2 + 1]);
      float2 uv1 = float2(UVBuffer[uvOffset + v1Idx * 2],
                          UVBuffer[uvOffset + v1Idx * 2 + 1]);
      float2 uv2 = float2(UVBuffer[uvOffset + v2Idx * 2],
                          UVBuffer[uvOffset + v2Idx * 2 + 1]);
      texCoords = w0 * uv0 + w1 * uv1 + w2 * uv2;
    }

    //float3x4 objToWorld = q.CommittedObjectToWorld3x4();

    //float3x3 toWorld;
    //toWorld[0] = objToWorld[0].xyz;
    //toWorld[1] = objToWorld[1].xyz;
    //toWorld[2] = objToWorld[2].xyz;

    //normal = mul(toWorld, normal);

    // compute position
    //position = mul(toWorld, position);
    //position += float3(objToWorld[0].w, objToWorld[1].w, objToWorld[2].w);
    
    pathState.normal = normal;
    pathState.position = position;
    pathState.outside = outside;
     
    //int matId = instanceOffset[instanceIndex].materialIdx;

    int diffuseTextureIdx = ubo0.texBuffer[instanceIndex].diffuseTextureIdx;
    if(diffuseTextureIdx != -1){
        pathState.albedo = textures[diffuseTextureIdx].Sample(linearSampler, texCoords).rgb;
    }
    else{
        pathState.albedo = float3(0.f, 0.f, 0.f);
    }

    int metallicAndRoughnessTextureIdx = ubo0.texBuffer[instanceIndex].metallicAndRoughnessTextureIdx;
    if(metallicAndRoughnessTextureIdx != -1){
        pathState.metallicAndRoughness = textures[metallicAndRoughnessTextureIdx].Sample(linearSampler, texCoords).rgb;
    }
    else{  
        pathState.metallicAndRoughness = float3(0.f, 0.f, 0.f);
    }

    pathState.uv = texCoords;

    return true;
  }

  return false;
} 

float3 DirectDiffuseLighting(Light light, float3 position, float3 normal, float3 albedo){
    float3 brdf = (albedo / (float)PI);
    float ndotv = max(dot(normal, -light.direction), 0.f);
    
    RayDesc rayDesc;
    rayDesc.Origin = position + normal * (1e-4);
    rayDesc.Direction = -light.direction;
    rayDesc.TMin = 0.f;
    rayDesc.TMax = 10000.f;
    float t;

    bool visibility = (1-RayTracingAnyHit(rayDesc, t));

    float3 lighting = light.color * light.intensity * (float)visibility * ndotv;
 
    return (brdf * lighting);
}

float3 SphericalFibonacci(float index, float numSamples)
{
    const float b = (sqrt(5.f) * 0.5f + 0.5f) - 1.f;
    float phi = 2 * PI * frac(index * b);
    float cosTheta = 1.f - (2.f * index + 1.f) * (1.f / numSamples);
    float sinTheta = sqrt(saturate(1.f - (cosTheta * cosTheta)));

    return float3((cos(phi) * sinTheta), (sin(phi) * sinTheta), cosTheta);
}

PSOutput main(PSInput input)
{
    const int2 FullResolution = int2(ubo2.raysPerProbe, 512);

    int2 idx = int2(FullResolution * input.texCoord);
    int rayIndex = idx.x;
    int probeIndex = idx.y;
    Probe probe = ubo1.probes[probeIndex];

    PSOutput output;
    output.albedo.w = 0.f;
    
    PathState pathState;
    {
        RayDesc ray;
        ray.TMin = 0.f;
        ray.TMax = 10000.f;
        ray.Origin = probe.position;
        //float3 randomDirection = SphericalFibonacci(rayIndex, 64);
        ray.Direction = SphericalFibonacci(rayIndex, ubo2.raysPerProbe);

        if(RayTracingClosestHit(ray, pathState)){
            output.position = float4(pathState.position, 1.f);
            output.normal = float4(pathState.normal, 1.f);
            output.albedo = float4(pathState.albedo, 1.f);
        } 
        else{ 
            output.position = float4(ray.Origin + ray.Direction * 10000.f, 1.f);
            output.normal = float4(0.f, 0.f, 0.f, 0.f);
            output.albedo = float4(0.f, 0.f, 0.f, 0.f);
        }
    }
   
    float3 diffuse = float3(0.f, 0.f, 0.f);
    for(int i=0;i<ubo2.lightNum;i++){
        if(output.normal.w > 0.f){
            diffuse += DirectDiffuseLighting(
                ubo2.lights[i], 
                output.position, 
                output.normal, 
                output.albedo); 
        }  
    }         
     
    if(output.normal.w > 0.f && pathState.outside){ 
    //if(output.normal.w > 0.f){
        IndirectLightingOutput indirectLight = CalculateIndirectLighting(
                    ubo1,
                    VolumeProbeDatasRadiance,  
                    VolumeProbeDatasDepth, 
                    linearSampler, 
                    ubo2.probePos0,
                    ubo2.probePos1,
                    output.position, 
                    output.normal); 
        diffuse += indirectLight.radiance * output.albedo / PI;
    }

    output.radiance = float4(diffuse, pathState.t);

    return output;
}
 