//#pragma once

#ifndef SHARED_COMMON_H
#define SHARED_COMMON_H

#ifdef SHARED_CODE_HLSL
#include "HLSLTypeAlise.h"
#define PI 3.1415926535f
#else
#pragma once
#include "Shaders/share/HLSLTypeAlise.h"
#endif

#define MAX_LIGHTS 4
#define MAX_TEXTURES 1024


enum class OutputContent{
    Render = 0,
    Depth = 1,
    Albedo = 2,
    Normal = 3,
    Position = 4,
    Roughness = 5,
    Metallic = 6,
    MotionVector = 7,
    ShadowMap = 8
};

struct MLight{
    float4x4 shadowViewProj;

    float3 direction;
    float intensity;

    float3 color;
    int shadowMapIndex;

    int2 shadowmapResolution;
    float shadowCameraZnear;
    float shadowCameraZfar;
};

struct LightBuffer{
    MLight lights[MAX_LIGHTS];

    //float3 cameraPos;
    int lightNum;
    int padding0;
    int padding1;
    int padding2;
};

struct MCameraBuffer {
    float3 cameraPos;
    float zNear;
    float3 cameraDir;
    float zFar;

    float fovY;
    float padding0;
    float padding1;
    float padding2;
};

struct MScreenBuffer {
    int2 WindowRes;
};

struct TexBuf
{
    int diffuseTextureIdx;
    int metallicAndRoughnessTextureIdx;
    int matId;
    int normalTextureIdx;

    float3 diffuseColor;
    int padding0;
};

using MaterialBuffer = TexBuf;

struct TexBuffer{
    TexBuf tex[MAX_TEXTURES];
};

struct MVPBuffer
{
    float4x4 Model;
    float4x4 View;
    float4x4 Projection;
};

struct ModelBuffer {
    float4x4 Model;
};

struct VPBuffer
{
    float4x4 View;
    float4x4 Projection;
};

#define CULLINGMODE_SPHERE 1
#define CULLINGMODE_AABB 2
#define CULLINGMODE_NULL 0

struct MSceneBuffer{
    int numInstances;
    int targetIndex;
    int cullingMode;
};

struct MDFGlobalBuffer{
    int3 mdfAtlasDim;
    float padding0;
};

struct InstanceBound{
    float3 center;
    float padding0;
    float3 extent;
    float padding1;
};

struct FrustumBuffer{
    float4 planes[6];
};

struct IndirectDrawArgs{
    int    indexCount;
    int    instanceCount;
    int    firstIndex;
    int    vertexOffset;
    int    firstInstance;

    float padding0;
    float padding1;
    float padding2;
    //float padding3;
    //float padding4;
    //float padding5;
    //float padding6;
};

struct HizDimension {
    uint2 u_previousLevelDimensions;
};

struct HizDimensionBuffer{
    uint2 hizDimensions[13];
};

struct HIZBuffer{
    int u_previousLevel; 
    uint2 u_previousLevelDimensions; 
    uint padding0;
};

struct SSRBuffer{
    uint min_traversal_occupancy;
    uint g_max_traversal_intersections;
};

#endif