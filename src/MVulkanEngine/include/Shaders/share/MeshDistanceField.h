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

    float3 volumeCenter;
    float volumeSurfaceBias;
    
    float3 volumeOffset;
    int firstBrickIndex;

    float3 volumeToWorldScale;
    float expandSurfaceDistance;

    float2 distanceFieldToVolumeScaleBias;
    float2 mdfTextureResolusion;
    //float padding1;
    //float padding2;

    int3 dimensions;
    float padding0;

    //float3 mdfTextureResolusion;
};

//struct MDFAsset{
//    int firstBrickIndex;
//    int3 
//};