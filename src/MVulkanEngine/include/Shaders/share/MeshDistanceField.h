#ifdef SHARED_CODE_HLSL
#include "HLSLTypeAlise.h"
#else
#pragma once
#include "Shaders/share/HLSLTypeAlise.h"
#endif

struct MeshDistanceFieldInput{
    float4x4 WorldToVolume;
    float4x4 VolumeToWorld;

    int3 numVoxels;
    float localToVolumnScale;

    float3 worldSpaceVolumeCenter;
    float volumeSurfaceBias;
    
    float3 worldSpaceVolumeExtent;
    int firstBrickIndex;

    float volumeToWorldScale;
    float expandSurfaceDistance;
    float padding1;
    float padding2;

    float2 distanceFieldToVolumeScaleBias;
    float2 mdfTextureResolusion;
    //float padding1;
    //float padding2;

    int3 dimensions;
    float padding0;

    //normalized into [-1,1]
    float3 volumeSpaceExtent;
    float padding3;

    //float3 mdfTextureResolusion;
};

//struct MDFAsset{
//    int firstBrickIndex;
//    int3 
//};