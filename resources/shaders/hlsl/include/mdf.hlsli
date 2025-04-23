#pragma once
#include "MeshDistanceField.h"
#include "SDF.hlsli"

struct MarchRay{
    float3 origin;
    float3 direction;
};

struct MDFHitPoint{
    float dis0;
    bool hitBox;
    float step;

    float3 volumeSpaceHitPosition;

    int instanceIndex;
    float distance;
    float3 position;
    float3 normal;
};

float SampleMeshDistanceField(
    //int meshDistanceFieldIndex,
    Texture3D meshDistanceFieldTexture,
    SamplerState sampler,
    MeshDistanceFieldInput meshDistanceFieldInput,
    float3 sampleVolumePosition
){
    float3 volumeMin = - meshDistanceFieldInput.volumeOffset;
    float3 volumeMax = meshDistanceFieldInput.volumeOffset;
    
    //const int brickSize = 8;
    //const int brickDim = meshDistanceFieldInput.dimensions / brickSize; 
    //const int brickUV = 

    float3 uv = (sampleVolumePosition - volumeMin) / (volumeMax - volumeMin);
    //int3 brickUV = (int3)(uv * meshDistanceFieldInput.dimensions);
//
    //int3 textureBrickUV = int3(brickUV.x * meshDistanceFieldInput.dimensions.y + brickUV.y, brickUV.z, 0);
    //float3 uvOffsetInt = (uv - brickUV) * brickSize * meshDistanceFieldInput.dimensions;
//
    //float3 finaluv = textureBrickUV * float3(brickSize*brickSize, brickSize, 0.f) / float3(meshDistanceFieldInput.mdfTextureResolusion, 1.f) + uvOffsetInt / float3(brickSize, brickSize, brickSize);
    //
    //float encodedMDFValue = meshDistanceFieldTexture.SampleLevel(sampler, finaluv, 0).r;
    float encodedMDFValue = meshDistanceFieldTexture.SampleLevel(sampler, uv, 0).r;
    //encodedMDFValue /= 255.f;
    float mdfValue = (float)encodedMDFValue * meshDistanceFieldInput.distanceFieldToVolumeScaleBias.x + meshDistanceFieldInput.distanceFieldToVolumeScaleBias.y;
    //mdfValue /= 255.f;

    return mdfValue;
}

void RayMarchingMDF(
    Texture3D meshDistanceFieldTexture,
    SamplerState sampler,
    MeshDistanceFieldInput meshDistanceFieldInput,
    MarchRay ray,
    out MDFHitPoint hitPoint
){
    hitPoint.distance = -1.f;
    hitPoint.step = 0;

    //float3 volumeSpaceRayOrigin = mul(meshDistanceFieldInput.WorldToVolume, float4(ray.origin, 1.0f)).xyz;
    //float3 volumeSpaceRayDirection = mul(meshDistanceFieldInput.WorldToVolume, float4(ray.direction, 1.0f)).xyz;
    float3 worldRayStart = ray.origin;
    float3 worldRayEnd = ray.origin + ray.direction * 1.0f;

    float3 volumeSpaceRayOrigin = mul(meshDistanceFieldInput.WorldToVolume, float4(worldRayStart, 1.0f)).xyz;
    float3 volumeSpaceRayEnd = mul(meshDistanceFieldInput.WorldToVolume, float4(worldRayEnd, 1.0f)).xyz;
    float3 volumeSpaceRayDirection = normalize(volumeSpaceRayEnd - volumeSpaceRayOrigin);

    float3 aabbMin = -meshDistanceFieldInput.volumeOffset;
    float3 aabbMax = meshDistanceFieldInput.volumeOffset;

    bool hit = false;
    float2 intersectBox = GetNearestDistance(
        volumeSpaceRayOrigin, volumeSpaceRayDirection, aabbMin, aabbMax, hit);

    hitPoint.dis0 = intersectBox.x;
    hitPoint.hitBox = hit;
    //hitPoint.distance = intersectBox.x;
    //return;

    if (hit){
        bool bHit = false;
        float distance = intersectBox.x;

        const uint MAXSTEPS = 64;

        int step = 0;
        for(; step < MAXSTEPS; step++){
            float3 volumeSpacePosition = volumeSpaceRayOrigin + volumeSpaceRayDirection * distance;
            float mdfValue = SampleMeshDistanceField(meshDistanceFieldTexture, sampler, meshDistanceFieldInput, volumeSpacePosition);

            //float ExpandSurfaceDistance = DFObjectData.VolumeSurfaceBias;
			//const float ExpandSurfaceFalloff = 2.0f * ExpandSurfaceDistance;
			//const float ExpandSurfaceAmount = ExpandSurfaceDistance * saturate(SampleRayTime / ExpandSurfaceFalloff);

            if(mdfValue < 0.005f){
                bHit = true;
                distance = clamp(distance, intersectBox.x, intersectBox.y);
                break;
            }

            distance += (mdfValue * 0.9f);

            if(distance > intersectBox.y){
                break;
            }
        }

        if(bHit || step == MAXSTEPS){
            const float newHitDistance = length(volumeSpaceRayDirection * distance * meshDistanceFieldInput.volumeToWorldScale);

            hitPoint.distance = newHitDistance;
        }
        hitPoint.step = (float)step / MAXSTEPS;
    }
}

float SampleMeshDistanceField2(
    Texture3D meshDistanceFieldTexture,
    SamplerState sampler,
    float3 atlasDim,
    MeshDistanceFieldInput meshDistanceFieldInput,
    float3 sampleVolumePosition
){
    //float3 atlasDim = float3(1024, 1024, 8); 
    int d = atlasDim.x / 8;

    float3 volumeMin = -meshDistanceFieldInput.volumeOffset;
    float3 volumeMax = meshDistanceFieldInput.volumeOffset;

    float3 volumnDim = meshDistanceFieldInput.dimensions * float3(8, 8, 8);
    float3 uv = (sampleVolumePosition - volumeMin) / (volumeMax - volumeMin);
    
    float3 uvInt = uv * volumnDim;
    int3 brickUV = (int3)(uv * meshDistanceFieldInput.dimensions);
    float3 uvInBrick = uvInt - brickUV * float3(8, 8, 8);
    uvInBrick = uvInBrick * 7.f / 8.f + 0.5f;
    //uvInBrick = 3.5f * (uvInBrick - 4.f) / 4.f + 3.5f;

    int localBrickIndex = (brickUV.x * meshDistanceFieldInput.dimensions.y + brickUV.y) * meshDistanceFieldInput.dimensions.z + brickUV.z;
    int globalBrickIndex = localBrickIndex + meshDistanceFieldInput.firstBrickIndex;

    int brickPosX = globalBrickIndex / d;
    int brickPosY = globalBrickIndex % d;
    float3 brickUVInt = float3(8, 8, 8) * float3(brickPosX, brickPosY, 0);

    float3 finalUV = (brickUVInt + uvInBrick) / atlasDim;

    float encodedMDFValue = meshDistanceFieldTexture.SampleLevel(sampler, finalUV, 0).r;
    float mdfValue = (float)encodedMDFValue * meshDistanceFieldInput.distanceFieldToVolumeScaleBias.x + meshDistanceFieldInput.distanceFieldToVolumeScaleBias.y;

    return mdfValue;
}

float3 CalculateMeshSDFGradient(
    Texture3D meshDistanceFieldTexture,
    SamplerState sampler,
    float3 atlasDim,
    MeshDistanceFieldInput meshDistanceFieldInput,
    float3 sampleVolumePosition)
{
    int DistanceFieldUniqueDataBrickSize = 7;
    float3 voxelSize = 1.f / float3(DistanceFieldUniqueDataBrickSize, DistanceFieldUniqueDataBrickSize, DistanceFieldUniqueDataBrickSize);//1.f / DistanceFieldUniqueDataBrickSize

    float3 volumeSpaceBrickSize = 2.f * meshDistanceFieldInput.volumeOffset / meshDistanceFieldInput.dimensions;
    float3 volumeSpaceVoxelSize = voxelSize * volumeSpaceBrickSize;

    float3 voxelOffset = 0.5f * volumeSpaceVoxelSize;
    float R = SampleMeshDistanceField2(meshDistanceFieldTexture, sampler, atlasDim, meshDistanceFieldInput, sampleVolumePosition + float3(voxelOffset.x, 0.f, 0.f));
    float L = SampleMeshDistanceField2(meshDistanceFieldTexture, sampler, atlasDim, meshDistanceFieldInput, sampleVolumePosition - float3(voxelOffset.x, 0.f, 0.f));
    float F = SampleMeshDistanceField2(meshDistanceFieldTexture, sampler, atlasDim, meshDistanceFieldInput, sampleVolumePosition + float3(0.f, voxelOffset.y, 0.f));
    float B = SampleMeshDistanceField2(meshDistanceFieldTexture, sampler, atlasDim, meshDistanceFieldInput, sampleVolumePosition - float3(0.f, voxelOffset.y, 0.f));
    float U = SampleMeshDistanceField2(meshDistanceFieldTexture, sampler, atlasDim, meshDistanceFieldInput, sampleVolumePosition + float3(0.f, 0.f, voxelOffset.z));
    float D = SampleMeshDistanceField2(meshDistanceFieldTexture, sampler, atlasDim, meshDistanceFieldInput, sampleVolumePosition - float3(0.f, 0.f, voxelOffset.z));

    float3 gradient = float3(R - L, F - B, U - D);
	return gradient;
}

void RayMarchingMDF2(
    Texture3D meshDistanceFieldTexture,
    SamplerState sampler,
    float3 atlasDim,
    MeshDistanceFieldInput meshDistanceFieldInput,
    MarchRay ray,
    out MDFHitPoint hitPoint
){
    hitPoint.distance = -1.f;
    hitPoint.step = 0;

    float3 worldRayStart = ray.origin;
    float3 worldRayEnd = ray.origin + ray.direction * 1.0f;

    float3 volumeSpaceRayOrigin = mul(meshDistanceFieldInput.WorldToVolume, float4(worldRayStart, 1.0f)).xyz;
    float3 volumeSpaceRayEnd = mul(meshDistanceFieldInput.WorldToVolume, float4(worldRayEnd, 1.0f)).xyz;
    float3 volumeSpaceRayDirection = normalize(volumeSpaceRayEnd - volumeSpaceRayOrigin);

    float3 aabbMin = -float3(1.f, 1.f, 1.f);
    float3 aabbMax = float3(1.f, 1.f, 1.f);
    //float3 aabbMin = meshDistanceFieldInput.volumeCenter - meshDistanceFieldInput.volumeOffset;
    //float3 aabbMax = meshDistanceFieldInput.volumeCenter + meshDistanceFieldInput.volumeOffset;

    bool hit = false;
    float2 intersectBox = GetNearestDistance(
        volumeSpaceRayOrigin, volumeSpaceRayDirection, aabbMin, aabbMax, hit);

    hitPoint.dis0 = intersectBox.x;
    hitPoint.hitBox = hit;
    //hitPoint.distance = intersectBox.x;
    //return;

    if (hit){
        bool bHit = false;
        float distance = intersectBox.x;

        const uint MAXSTEPS = 64;

        int step = 0;
        for(; step < MAXSTEPS; step++){
            float3 volumeSpacePosition = volumeSpaceRayOrigin + volumeSpaceRayDirection * distance;
            float mdfValue = SampleMeshDistanceField2(meshDistanceFieldTexture, sampler, atlasDim, meshDistanceFieldInput, volumeSpacePosition);

            float ExpandSurfaceDistance = meshDistanceFieldInput.expandSurfaceDistance;
			const float ExpandSurfaceFalloff = 2.0f * ExpandSurfaceDistance;
			const float ExpandSurfaceAmount = ExpandSurfaceDistance * saturate(distance / ExpandSurfaceFalloff);

            if(mdfValue < ExpandSurfaceAmount){
            //if(mdfValue < 1.f){
                bHit = true;
                distance = clamp(distance, intersectBox.x, intersectBox.y);
                break;
            }

            distance += (mdfValue * 0.95f);

            if(distance > intersectBox.y){
                break;
            }
        }

        if(bHit || step == MAXSTEPS){
            const float newHitDistance = length(volumeSpaceRayDirection * distance * meshDistanceFieldInput.volumeToWorldScale);

            hitPoint.distance = newHitDistance;

            float3 volumeSpacePosition = volumeSpaceRayOrigin + volumeSpaceRayDirection * distance;
            hitPoint.volumeSpaceHitPosition = volumeSpacePosition;
        }
        hitPoint.step = (float)step / MAXSTEPS;
    }
}

