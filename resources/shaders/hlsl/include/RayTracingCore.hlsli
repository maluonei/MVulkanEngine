#pragma once
[[vk::binding(1, 0)]] RaytracingAccelerationStructure Tlas;
[[vk::binding(2, 0)]] StructuredBuffer<float> VertexBuffer;
[[vk::binding(3, 0)]] StructuredBuffer<int> IndexBuffer;

[[vk::binding(5, 0)]] StructuredBuffer<GeometryInfo> instanceOffset;
[[vk::binding(6, 0)]] StructuredBuffer<float> NormalBuffer;
[[vk::binding(8, 0)]] StructuredBuffer<float> UVBuffer;

struct PathState {
  float3 position;
  float3 normal;
  float3 emission;
  int materialIdx;
  float2 uv;

  uint instanceID;
  uint primitiveID;
};

bool RayTracingClosestHit(inout RayDesc rayDesc, inout PathState pathState) {
  uint rayFlags = RAY_FLAG_FORCE_OPAQUE;

  RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;

  q.TraceRayInline(Tlas, rayFlags, 0xFF, rayDesc);
  q.Proceed();

  if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
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
    float2 texCoords = 0.f.xx;

    if (uvOffset != -1) {
      float2 uv0 = float2(UVBuffer[uvOffset + v0Idx * 3],
                          UVBuffer[uvOffset + v0Idx * 3 + 1]);
      float2 uv1 = float2(UVBuffer[uvOffset + v1Idx * 3],
                          UVBuffer[uvOffset + v1Idx * 3 + 1]);
      float2 uv2 = float2(UVBuffer[uvOffset + v2Idx * 3],
                          UVBuffer[uvOffset + v2Idx * 3 + 1]);
      texCoords = w0 * uv0 + w1 * uv1 + w2 * uv2;
    }

    float3x4 objToWorld = q.CommittedObjectToWorld3x4();

    float3x3 toWorld;
    toWorld[0] = objToWorld[0].xyz;
    toWorld[1] = objToWorld[1].xyz;
    toWorld[2] = objToWorld[2].xyz;

    normal = mul(toWorld, normal);

    // compute position
    position = mul(toWorld, position);
    position += float3(objToWorld[0].w, objToWorld[1].w, objToWorld[2].w);

    pathState.normal = normal;
    pathState.position = position;
    pathState.emission = float3(instanceOffset[instanceIndex].emission[0],
                                instanceOffset[instanceIndex].emission[1],
                                instanceOffset[instanceIndex].emission[2]);
    pathState.materialIdx = instanceOffset[instanceIndex].materialIdx;
    pathState.uv = texCoords;

    return true;
  }

  return false;
}

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