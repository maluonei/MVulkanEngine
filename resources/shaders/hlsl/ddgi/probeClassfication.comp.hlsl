#include "Common.h"
#include "indirectLight.hlsli"

//struct UniformBuffer0{
//    float maxRayDistance;
//    int padding1;
//    int padding2;
//    int padding3;
//};
//
//[[vk::binding(0, 0)]]
//cbuffer ubo : register(b0)
//{
//    UniformBuffer0 ubo0;
//}; 

[[vk::binding(1, 0)]]
cbuffer ddgiBuffer : register(b1)
{
    DDGIBuffer ubo1;
};

[[vk::binding(2, 0)]]RWStructuredBuffer<DDGIProbe> probes : register(u0);
[[vk::binding(3, 0)]]Texture2D<float4> VolumeProbePosition : register(t0);  //[raysPerProbe, probeDim.x*probeDim.y*probeDim.z]
[[vk::binding(4, 0)]]Texture2D<float4> VolumeProbeRadiance : register(t1);  //[raysPerProbe, probeDim.x*probeDim.y*probeDim.z]
[[vk::binding(5, 0)]]RWTexture2D<float4> TestTexture : register(u1);  //[raysPerProbe, probeDim.x*probeDim.y*probeDim.z]

#define ProbeFixedRayBackfaceThreshold 0.1
#define MaxPaysPerProbe 256

[numthreads(32, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint probeIndex = DispatchThreadID.x;
    //probes[probeIndex].probeState = PROBE_STATE_INACTIVE;
    int numRays = ubo1.raysPerProbe;
    int backfaceCount = 0;

    TestTexture[uint2(probeIndex, 0)] = float4(0.f, 0.f, 0.f, 1.f);

    float hitDistances[MaxPaysPerProbe];
    for(int i=0; i<numRays; i++){
        int2 texcoord = int2(i, probeIndex);
        hitDistances[i] = VolumeProbeRadiance[texcoord].a;
        backfaceCount += (hitDistances[i] < 0.f);
    } 

    if((float)backfaceCount / (float)numRays > ProbeFixedRayBackfaceThreshold){
        probes[probeIndex].probeState = PROBE_STATE_INACTIVE;
        TestTexture[uint2(probeIndex, 0)] = float4(1.f, 0.f, 0.f, 1.f);
        return;
    }  
  
    float3 probePosition = GetProbePosition(ubo1, probes[probeIndex]);
    float3 probeSpacing = (ubo1.probePos1 - ubo1.probePos0) / float3(ubo1.probeDim);

    for (int i = 0; i < numRays; i++) {
        if (hitDistances[i] > ubo1.maxRayDistance) continue;

        int2 texcoord = int2(i, probeIndex);
        float3 hitPosition = VolumeProbePosition[texcoord].xyz;
        float3 direction = normalize(hitPosition - probePosition);

        // Get the plane normals
        float3 xNormal = float3(direction.x / max(abs(direction.x), 0.000001f), 0.f, 0.f);
        float3 yNormal = float3(0.f, direction.y / max(abs(direction.y), 0.000001f), 0.f);
        float3 zNormal = float3(0.f, 0.f, direction.z / max(abs(direction.z), 0.000001f));

        // Get the relevant planes to intersect
        float3 p0x = probePosition + (probeSpacing.x * xNormal);
        float3 p0y = probePosition + (probeSpacing.y * yNormal);
        float3 p0z = probePosition + (probeSpacing.z * zNormal);

        // Get the ray's intersection distance with each plane
        float3 distances = 
        {
            dot((p0x - probePosition), xNormal) / max(dot(direction, xNormal), 0.000001f),
            dot((p0y - probePosition), yNormal) / max(dot(direction, yNormal), 0.000001f),
            dot((p0z - probePosition), zNormal) / max(dot(direction, zNormal), 0.000001f)
        };

        // If the ray is parallel to the plane, it will never intersect
        // Set the distance to a very large number for those planes
        if (distances.x == 0.f) distances.x = 1e27f;
        if (distances.y == 0.f) distances.y = 1e27f;
        if (distances.z == 0.f) distances.z = 1e27f;

        // Get the distance to the closest plane intersection
        float maxDistance = min(distances.x, min(distances.y, distances.z));

        // If the hit distance is less than the closest plane intersection, the probe should be active
        if(hitDistances[i] <= maxDistance)
        {
            probes[probeIndex].probeState = PROBE_STATE_ACTIVE;
            return;
        }
    }

    probes[probeIndex].probeState = PROBE_STATE_INACTIVE;
    TestTexture[uint2(probeIndex, 0)] = float4(1.f, 0.f, 0.f, 1.f);
    //probes[5262].probeState = PROBE_STATE_INACTIVE;
}