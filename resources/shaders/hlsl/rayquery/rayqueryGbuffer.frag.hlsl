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

struct UniformBuffer1
{
    float3 cameraPosition;
    int    lightNum;

    float3 cameraDirection;
    int    padding0;

    int2   screenResolution;
    float  fovY;
    float  zNear;
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

[[vk::binding(2, 0)]] StructuredBuffer<float> VertexBuffer : register(t0);
[[vk::binding(3, 0)]] StructuredBuffer<int> IndexBuffer : register(t1);
[[vk::binding(4, 0)]] StructuredBuffer<float> NormalBuffer : register(t2);
[[vk::binding(5, 0)]] StructuredBuffer<float> UVBuffer : register(t3);
[[vk::binding(6, 0)]] StructuredBuffer<GeometryInfo> instanceOffset : register(t4);
[[vk::binding(7, 0)]] Texture2D<float4> textures[1024] : register(t5);

[[vk::binding(8, 0)]] SamplerState linearSampler : register(s0);

[[vk::binding(9, 0)]] RaytracingAccelerationStructure Tlas : register(t1032);

#define PI 3.14159265359f

struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Normal : SV_TARGET0;
    [[vk::location(1)]] float4 Position : SV_TARGET1;
    [[vk::location(2)]] float4 Albedo : SV_TARGET2;
    [[vk::location(3)]] float4 MetallicAndRoughness : SV_TARGET3;
    [[vk::location(4)]] uint4 MatId : SV_TARGET4;
};

//static const float PI = 3.14159265359f;

struct PathState {
  float3 position;
  float3 normal;
  float3 albedo;
  float3 metallicAndRoughness;

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

    float3 normal = normalize(cross(e0, e1));

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

PSOutput main(PSInput input)
{
    const float3 UP = float3(0.f, 1.f, 0.f);

    float halfHeight = ubo1.zNear * tan(ubo1.fovY * 0.5f);
    float halfWidth = ubo1.screenResolution.x * halfHeight * (1.f / ubo1.screenResolution.y);
    
    float3 forward = normalize(ubo1.cameraDirection);
    float3 right = normalize(cross(forward, UP));
    float3 up = normalize(cross(right, forward));

    float3 offsetx = 2.f * (input.texCoord.x - 0.5f) * halfWidth * right;
    float3 offsety = - 2.f * (input.texCoord.y - 0.5f) * halfHeight * up;
    float3 offsetz = forward * ubo1.zNear;

    float3 direction = normalize(offsetx + offsety + offsetz);

    PSOutput output;
    output.Albedo.w = 0.f;
    
    PathState pathState;
    {
        RayDesc ray;
        ray.TMin = ubo1.zNear;
        ray.TMax = 1000.f;
        ray.Origin = ubo1.cameraPosition;
        ray.Direction = direction;

        if(RayTracingClosestHit(ray, pathState)){
            output.Position = float4(pathState.position, 1.f);
            output.Normal = float4(pathState.normal, 1.f);
            output.Albedo = float4(pathState.albedo, 1.f);
            output.MetallicAndRoughness = float4(pathState.metallicAndRoughness, 1.f);
        }
        else{
            output.Position = float4(0.f, 0.f, 0.f, 0.f);
            output.Normal = float4(0.f, 0.f, 0.f, 0.f);
            output.Albedo = float4(0.f, 0.f, 0.f, 0.f);
            output.MetallicAndRoughness = float4(0.f, 0.f, 0.f, 0.f);
        }
    }

    return output;
}
