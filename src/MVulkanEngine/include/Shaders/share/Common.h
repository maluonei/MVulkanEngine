#ifdef SHARED_CODE_HLSL
#include "HLSLTypeAlise.h"
#else
#pragma once
#include "Shaders/share/HLSLTypeAlise.h"
#endif

#define MAX_LIGHTS 4
#define MAX_TEXTURES 1024

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

struct TexBuffer{
    TexBuf tex[MAX_TEXTURES];
};

struct MVPBuffer
{
    float4x4 Model;
    float4x4 View;
    float4x4 Projection;
};
