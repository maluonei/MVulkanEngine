#define SHARED_CODE_HLSL
#include "util.hlsli"
#include "mdf.hlsli"
//#include "common.h"

struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET0;
};

//[[vk::binding(0, 0)]]
//cbuffer mdfBuffer : register(b0)
//{
//    MeshDistanceFieldInput mdfInput;
//};
 
[[vk::binding(1, 0)]]
cbuffer cameraBuffer : register(b1)
{
    MCameraBuffer cam;
};
 
[[vk::binding(2, 0)]]
cbuffer screenBuffer : register(b2)
{
    MScreenBuffer screen;
}; 

[[vk::binding(0, 0)]]
cbuffer sceneBuffer : register(b0)
{
    MSceneBuffer scene;
}; 

[[vk::binding(6, 0)]]
cbuffer mdfGlobalBuffer : register(b3)
{
    MDFGlobalBuffer mdfGlobalBuffer;
}; 

[[vk::binding(3, 0)]] StructuredBuffer<MeshDistanceFieldInput> mdfInputs : register(t2);

[[vk::binding(4, 0)]] Texture3D MDFAtlas : register(t0);
[[vk::binding(5, 0)]] SamplerState trilinerSampler : register(s0);

PSOutput main(PSInput input)
{
    PSOutput output;
    //RayMarchSDFStruct struc;

    //struc.step = 0;
    float3 camDir = GetPixelDirection(
        cam,
        screen,
        input.texCoord
    );

    const float MAX_RAY_DISTANCE = 1e+10;
    //float finalDistance = MAX_RAY_DISTANCE;
    MDFHitPoint finalHit;
    finalHit.distance = MAX_RAY_DISTANCE;
    float3 rayOrigin = cam.cameraPos;
    float3 rayDir = camDir;
    
    for(int i=0; i<scene.numInstances; i++){
    //for(int i=min(scene.numInstances, 5); i<min(scene.numInstances, 6); i++){
        MDFHitPoint hit;
        MarchRay ray;
        ray.origin = rayOrigin;
        ray.direction = rayDir;

        MeshDistanceFieldInput mdfInput = mdfInputs[i];
        RayMarchingMDF2(MDFAtlas, trilinerSampler, mdfGlobalBuffer.mdfAtlasDim, mdfInput, ray, hit);

        if(hit.distance > 0 && finalHit.distance > hit.distance){
            finalHit = hit;
            finalHit.instanceIndex = i;
        }
    }

    if(finalHit.distance > 0 && finalHit.distance < MAX_RAY_DISTANCE){
        float3 hitPosition = finalHit.distance * rayDir + rayOrigin;
        
        MeshDistanceFieldInput mdfInput = mdfInputs[finalHit.instanceIndex];
        float3 volumeSpaceGradient = CalculateMeshSDFGradient(
            MDFAtlas, trilinerSampler, mdfGlobalBuffer.mdfAtlasDim, mdfInput, finalHit.volumeSpaceHitPosition
        );
        float3 volumeSpaceNormal = length(volumeSpaceGradient) > 0.f? volumeSpaceGradient : float3(0.f, 0.f, 0.f);
        float3 worldSpaceNormal = normalize(mul(transpose(inverse((float3x3)mdfInput.WorldToVolume)), volumeSpaceNormal));
        //float3 worldSpaceNormal = normalize(mul(transpose(mdfInput.VolumeToWorld), volumeSpaceNormal));

        //output.Color = float4(1.f, 1.f, 1.f, 1.f) + 1e-10f * float4(finalHit.distance, 0.f, 0.f, 1.f);
        //output.Color = float4(hitPosition, 1.f);// + 1e-10f * float4(finalDistance, 0.f, 0.f, 1.f);
        //output.Color = float4(worldSpaceNormal, 1.f);
        output.Color = float4(worldSpaceNormal * 0.5f + 0.5f, 1.f);
    }
    else{
        output.Color = float4(0.f, 0.f, 0.f, 1.f);
    }

    return output;
}