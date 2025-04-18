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

[[vk::binding(0, 0)]]
cbuffer mdfBuffer : register(b0)
{
    MeshDistanceFieldInput mdfInput;
};
 
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

[[vk::binding(3, 0)]] Texture3D MDFTexture : register(t0);
[[vk::binding(4, 0)]] SamplerState trilinerSampler : register(s0);

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
    
    MDFHitPoint hit;
    MarchRay ray;
    ray.origin = cam.cameraPos;
    ray.direction = camDir;

    RayMarchingMDF(MDFTexture, trilinerSampler, mdfInput, ray, hit);
    
    //if(hit.hitBox){ 
    if(hit.distance > 0){
        float3 hitPosition = hit.distance * ray.direction + ray.origin;
        //output.Color = hit.step * float4(1.f, 1.f, 1.f, 1.f) + 1e-10f * float4(hit.distance, 0.f, 0.f, 1.f);
        output.Color = float4(hitPosition, 1.f) + 1e-10f * float4(hit.distance, 0.f, 0.f, 1.f);
    }
    else{
        //output.Color = hit.step * float4(1.f, 1.f, 1.f, 1.f) + 1e-10f * float4(hit.distance, 0.f, 0.f, 1.f);

        output.Color = float4(0.f, 0.f, 0.f, 1.f);
    }

    //output.Color = hit.step * float4(1.f, 1.f, 1.f, 1.f) + 1e-10f * output.Color;
    //output.Color = hit.distance * float4(1.f, 1.f, 1.f, 1.f) + 1e-10f * output.Color;
    //output.Color = 1e-10f * output.Color + float4(ray.direction, 1.f);

    return output;
}