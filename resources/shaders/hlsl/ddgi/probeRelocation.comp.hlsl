#include "indirectLight.hlsli"

// HLSL Shader
struct ModelBuffer
{
    float4x4 Model;
};

[[vk::binding(0, 0)]]
cbuffer ubo1 : register(b0)
{
    UniformBuffer1 ubo1;
};
  
[[vk::binding(1, 0)]] RWStructuredBuffer<Probe> probes : register(u0);
[[vk::binding(2, 0)]] RWStructuredBuffer<ModelBuffer> Models : register(u1);
[[vk::binding(3, 0)]] Texture2D<float4> VolumeProbePosition : register(t2);
[[vk::binding(4, 0)]] Texture2D<float4> VolumeProbeRadiance : register(t3);


#define DepthProbeResolution 16
#define ProbeFixedRayBackfaceThreshold 0.1f

float3 GetProbeRayDirection(int rayIndex, int probeIndex){
    float3 targetPos = VolumeProbePosition.Load(int3(rayIndex, probeIndex, 0)).xyz;
    float3 probePos = probes[probeIndex].position;
    float3 offset = probes[probeIndex].offset;
    float3 direction = normalize(targetPos - (probePos + offset));
    return direction;
}

[numthreads(32, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint probeIndex = DispatchThreadID.x;
    probes[probeIndex].moved = 0;

    int numProbes = ubo1.probeDim.x * ubo1.probeDim.y * ubo1.probeDim.z;

    if(probeIndex >= numProbes)
        return;

    float3 probePosition = probes[probeIndex].position;
    float3 offset = probes[probeIndex].offset;

    int   closestBackfaceIndex = -1;
    int   closestFrontfaceIndex = -1;
    int   farthestFrontfaceIndex = -1;
    float closestBackfaceDistance = 1e27f;
    float closestFrontfaceDistance = 1e27f;
    float farthestFrontfaceDistance = 0.f;
    float backfaceCount = 0.f;

    for (int rayIndex = 0; rayIndex < ubo1.raysPerProbe; rayIndex++)
    {
        // Get the coordinates for the probe ray in the RayData texture array
        //int3 rayDataTexCoords = DDGIGetRayDataTexelCoords(rayIndex, probeIndex, volume);

        // Load the hit distance for the ray
        //float hitDistance = DDGILoadProbeRayDistance(RayData, rayDataTexCoords, volume);
        //uint2 probeBaseCoords = uint2(probeIndex % ubo1.probeDim.z, probeIndex / ubo1.probeDim.z);
        //probeBaseCoords = probeBaseCoords * uint2(DepthProbeResolution, DepthProbeResolution) + uint2(DepthProbeResolution, DepthProbeResolution) / 2;

        float hitDistance = VolumeProbeRadiance.Load(int3(rayIndex, probeIndex, 0)).a;

        if (hitDistance < 0.f)
        {
            // Found a backface
            backfaceCount++;

            // Negate the hit distance on a backface hit and scale back to the full distance
            hitDistance = hitDistance * -1.f;
            if (hitDistance < closestBackfaceDistance)
            {
                // Store the closest backface distance and ray index
                closestBackfaceDistance = hitDistance;
                closestBackfaceIndex = rayIndex;
            }
        }
        else if(hitDistance > 9999.f){ //miss
            continue;
        }
        else
        {
            // Found a frontface
            if (hitDistance < closestFrontfaceDistance)
            {
                // Store the closest frontface distance and ray index
                closestFrontfaceDistance = hitDistance;
                closestFrontfaceIndex = rayIndex;
            }
            else if (hitDistance > farthestFrontfaceDistance)
            {
                // Store the farthest frontface distance and ray index
                farthestFrontfaceDistance = hitDistance;
                farthestFrontfaceIndex = rayIndex;
            }
        }
    }

    float3 fullOffset = float3(1e27f, 1e27f, 1e27f);
    //float3 offset = float3(0.f, 0.f, 0.f);

    if (closestBackfaceIndex != -1 && ((float)backfaceCount / ubo1.raysPerProbe) > ProbeFixedRayBackfaceThreshold)
    {
        //float3 closestBackfacePos = VolumeProbePosition.Load(int3(closestBackfaceIndex, probeIndex, 0)).xyz;
        float3 closestBackfaceDirection = GetProbeRayDirection(closestBackfaceIndex, probeIndex);
        fullOffset = offset + closestBackfaceDirection * (closestBackfaceDistance + ubo1.minFrontFaceDistance * 0.5f);
    }
    else if (closestFrontfaceDistance < ubo1.minFrontFaceDistance){
        //float3 closestFrontfacePos = VolumeProbePosition.Load(int3(closestFrontfaceIndex, probeIndex, 0)).xyz;
        float3 closestFrontfaceDirection = GetProbeRayDirection(closestFrontfaceIndex, probeIndex);
        float3 farthestFrontfaceDirection = GetProbeRayDirection(farthestFrontfaceIndex, probeIndex);

        //probes[probeIndex].farthestFrontfaceDistance = farthestFrontfaceDistance;
        //probes[probeIndex].closestFrontfaceDistance = closestFrontfaceDistance;
        //probes[probeIndex].closestBackfaceDistance = closestBackfaceDistance;
        //probes[probeIndex].closestFrontfaceDirection = closestFrontfaceDirection;
        //probes[probeIndex].farthestFrontfaceDirection = farthestFrontfaceDirection;

        if(dot(closestFrontfaceDirection, farthestFrontfaceDirection) <= 0.f){
            farthestFrontfaceDirection *= min(farthestFrontfaceDistance, 1.f);
            fullOffset = offset + farthestFrontfaceDirection;
        }
    }
    else if (closestFrontfaceDistance > ubo1.minFrontFaceDistance){
        float moveBackMargin = min(closestFrontfaceDistance - ubo1.minFrontFaceDistance, length(offset));
        float3 moveBackDirection = normalize(-offset);
        fullOffset = offset + (moveBackMargin * moveBackDirection);
    }

    float3 probeSpacing = (ubo1.probePos1 - ubo1.probePos0) / float3(ubo1.probeDim - 1);
    float3 normalizedOffset = fullOffset / probeSpacing;
    if (dot(normalizedOffset, normalizedOffset) < 0.2025f) // 0.45 * 0.45 == 0.2025
    {
        offset = fullOffset;
        probes[probeIndex].moved = 1;
    } 
    //probes[probeIndex].padding1 = fullOffset;
    //probes[probeIndex].padding0 = farthestFrontfaceDistance;
    //probes[probeIndex].padding2 = closestFrontfaceDistance;

    probes[probeIndex].offset = offset;
    
    float3 neWPosition = probePosition + offset;
     
    ModelBuffer model;
    model.Model = float4x4(
        0.05f, 0.f,  0.f,  neWPosition.x, 
        0.f,  0.05f, 0.f,  neWPosition.y, 
        0.f,  0.f,  0.05f, neWPosition.z, 
        0.f,  0.f,  0.f,  1.f);
    Models[probeIndex] = model;
}