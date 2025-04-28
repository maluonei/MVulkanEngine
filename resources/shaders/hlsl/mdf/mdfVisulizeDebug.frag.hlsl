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
    //[[vk::location(1)]] float4 Color_Debug1 : SV_TARGET1;
    //[[vk::location(2)]] float4 Color_Debug2 : SV_TARGET2;
    //[[vk::location(3)]] float4 Color_Debug3 : SV_TARGET3;
    //[[vk::location(4)]] float4 Color_Debug4 : SV_TARGET4;
    //[[vk::location(5)]] float4 Color_Debug5 : SV_TARGET5;
    //[[vk::location(6)]] float4 Color_Debug6 : SV_TARGET6;
    //[[vk::location(7)]] float4 Color_Debug7 : SV_TARGET7;
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
[[vk::binding(7, 0)]] Texture3D MDFAtlasOrigin : register(t1);
[[vk::binding(5, 0)]] SamplerState trilinerSampler : register(s0);

PSOutput main(PSInput input)
{
    PSOutput output;
    //output.Color_Debug1 = float4(0.f, 0.f, 0.f, 1.f);
    //output.Color_Debug2 = float4(0.f, 0.f, 0.f, 1.f);
    //output.Color_Debug3 = float4(0.f, 0.f, 0.f, 1.f);
    //output.Color_Debug4 = float4(0.f, 0.f, 0.f, 1.f);
    //output.Color_Debug5 = float4(0.f, 0.f, 0.f, 1.f);
    //output.Color_Debug6 = float4(0.f, 0.f, 0.f, 1.f);
    //output.Color_Debug7 = float4(0.f, 0.f, 0.f, 1.f);
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
    finalHit.hitBoxButMiss = false;
    float3 rayOrigin = cam.cameraPos;
    float3 rayDir = camDir;

    float v = MDFAtlasOrigin.SampleLevel(trilinerSampler, float3(input.texCoord, 8.f / 14.f), 0).r;

    //for(int i=0;i<7;i++){
    //    for(int j=min(scene.numInstances, i); j<min(scene.numInstances, i+1); j++){
    //        MDFHitPoint hit;
    //        hit.hitBoxButMiss = false;
    //        MarchRay ray;
    //        ray.origin = rayOrigin;
    //        ray.direction = rayDir;
    //        ray.tMax = finalHit.distance;
//
    //        MeshDistanceFieldInput mdfInput = mdfInputs[j];
    //        RayMarchingMDF2(MDFAtlas, trilinerSampler, mdfGlobalBuffer.mdfAtlasDim, mdfInput, ray, hit);
//
    //        if(hit.distance > 0){
    //            float3 hitPosition = hit.distance * rayDir + rayOrigin;
    //            float3 worldSpaceNormal;
    //            //MeshDistanceFieldInput mdfInput = mdfInputs[hit.instanceIndex];
    //            float3 volumeSpaceGradient = CalculateMeshSDFGradient(
    //                MDFAtlas, trilinerSampler, mdfGlobalBuffer.mdfAtlasDim, mdfInput, hit.volumeSpaceHitPosition
    //            );
    //            if(isnan(volumeSpaceGradient.x) || isnan(volumeSpaceGradient.y) || isnan(volumeSpaceGradient.z)){
    //                worldSpaceNormal = float3(0.f, 0.f, 0.f);
    //            }
    //            else{
    //                float3 volumeSpaceNormal = length(volumeSpaceGradient) > 0.f? volumeSpaceGradient : float3(0.f, 0.f, 0.f);
    //                worldSpaceNormal = normalize(mul(transpose(inverse((float3x3)mdfInput.WorldToVolume)), volumeSpaceNormal));
    //                if(isnan(worldSpaceNormal.x) || isnan(worldSpaceNormal.y) || isnan(worldSpaceNormal.z)){
    //                    worldSpaceNormal = float3(0.f, 0.f, 0.f);
    //                }
    //            }
    //
    //            if(i==0){
    //                output.Color_Debug1 = float4(worldSpaceNormal, 1.f);
    //            }
    //            else if(i==1){
    //                output.Color_Debug2 = float4(worldSpaceNormal, 1.f);
    //            }
    //            else if(i==2){
    //                output.Color_Debug3 = float4(worldSpaceNormal, 1.f);
    //            }
    //            else if(i==3){
    //                output.Color_Debug4 = float4(worldSpaceNormal, 1.f);
    //            }
    //            else if(i==4){
    //                output.Color_Debug5 = float4(worldSpaceNormal, 1.f);
    //            }
    //            else if(i==5){
    //                output.Color_Debug6 = float4(worldSpaceNormal, 1.f);
    //            }
    //            else if(i==6){
    //                output.Color_Debug7 = float4(worldSpaceNormal, 1.f);
    //            }
    //            
    //        }
//
    //    }
    //}

    //for(int i=0; i<scene.numInstances; i++){
    int startIndex;
    int endIndex;
    if(scene.targetIndex == scene.numInstances){
        startIndex = 0;
        endIndex = scene.numInstances;
    }
    else{
        startIndex = min(max(0, scene.targetIndex), scene.numInstances);
        endIndex = min(max(0, scene.targetIndex+1), scene.numInstances);
    }
    //int targetIndex = max(0, scene.targetIndex);
    for(int i=startIndex; i<endIndex; i++){
        MDFHitPoint hit;
        hit.hitBoxButMiss = false;
        MarchRay ray;
        ray.origin = rayOrigin;
        ray.direction = rayDir;
        ray.tMax = finalHit.distance;

        MeshDistanceFieldInput mdfInput = mdfInputs[i];
        RayMarchingMDF2(MDFAtlas, trilinerSampler, mdfGlobalBuffer.mdfAtlasDim, mdfInput, ray, hit);

        if(hit.distance > 0 && finalHit.distance > hit.distance){
            finalHit = hit;
            finalHit.instanceIndex = i;
            finalHit.distance = hit.distance;
            finalHit.step = hit.step;
        }
    }

    if(finalHit.distance > 0 && finalHit.distance < MAX_RAY_DISTANCE){
        float3 hitPosition = finalHit.distance * rayDir + rayOrigin;
        
        MeshDistanceFieldInput mdfInput = mdfInputs[finalHit.instanceIndex];
        float3 volumeSpaceGradient = CalculateMeshSDFGradient(
            MDFAtlas, trilinerSampler, mdfGlobalBuffer.mdfAtlasDim, mdfInput, finalHit.volumeSpaceHitPosition
        );

        float3 worldSpaceNormal;
        if(isnan(volumeSpaceGradient.x) || isnan(volumeSpaceGradient.y) || isnan(volumeSpaceGradient.z)){
            worldSpaceNormal = float3(0.f, 0.f, 0.f);
            output.Color = float4(worldSpaceNormal * 0.5f + 0.5f, 1.f + v);
        }
        else{

            float3 volumeSpaceNormal = length(volumeSpaceGradient) > 0.f? volumeSpaceGradient : float3(0.f, 0.f, 0.f);
            worldSpaceNormal = normalize(mul(transpose(inverse((float3x3)mdfInput.WorldToVolume)), volumeSpaceNormal));
            //float3 worldSpaceNormal = normalize(mul(transpose(mdfInput.VolumeToWorld), volumeSpaceNormal));

            if(isnan(worldSpaceNormal.x) || isnan(worldSpaceNormal.y) || isnan(worldSpaceNormal.z)){
                worldSpaceNormal = float3(0.f, 0.f, 0.f);
            }

            //if(finalHit.hitBoxButMiss){
            //    output.Color = float4(0.f, 1.f, 0.f, 1.f);
            //}
            //else{
            //output.Color = float4(finalHit.distance / 20.f * float3(1.f, 1.f, 1.f), 1.f + v);
            //output.Color = float4(finalHit.step * float3(1.f, 1.f, 1.f), 1.f + v);
            //output.Color = float4(1.f, 1.f, 1.f, 1.f) + 1e-10f * float4(finalHit.distance, 0.f, 0.f, 1.f);
            //output.Color = float4(hitPosition, 1.f);// + 1e-10f * float4(finalDistance, 0.f, 0.f, 1.f);
            //output.Color = float4(worldSpaceNormal, 1.f);
            output.Color = float4(worldSpaceNormal * 0.5f + 0.5f, 1.f + v);
        }
        //}
    }
    else{
        output.Color = float4(0.f, 0.f, 0.f, 1.f);
    }

    //output.Color_Debug1 = float4(v * float3(1.f, 1.f, 1.f), 1.f);

    return output;
}