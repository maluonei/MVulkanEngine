#include "Common.h"
#include "shading.hlsli"
#include "util.hlsli"

[[vk::binding(0, 0)]]
cbuffer lightBuffer : register(b0)
{
    LightBuffer lightBuffer;
}

[[vk::binding(1, 0)]]
cbuffer cameraBuffer : register(b1)
{
    MCameraBuffer cameraBuffer;
}

[[vk::binding(2, 0)]]
cbuffer screenBuffer : register(b2)
{
    MScreenBuffer screenBuffer;
}

[[vk::binding(7, 0)]]
cbuffer intBuffer : register(b3)
{
    //int outputMotionVector;
    OutputContent outputContent;
}

//#define ShadingShaderConstantBuffer ubo0;

[[vk::binding(3, 0)]]Texture2D<uint4> gBuffer0 : register(t0);
[[vk::binding(4, 0)]]Texture2D<uint4> gBuffer1 : register(t1);
//[[vk::binding(8, 0)]]Texture2D<uint4> gBufferDepth : register(t2);
//[[vk::binding(8, 0)]]Texture2D<float4> gBuffer2 : register(t2);
[[vk::binding(5, 0)]]Texture2D<float> shadowMaps[2] : register(t4);
[[vk::binding(6, 0)]]SamplerState linearSampler : register(s0);

struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
    //float  depth : SV_Depth;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

//float3 DecodeNormalOctahedron(float2 enc)
//{
//    float3 n = float3(enc.xy, 1.0f - abs(enc.x) - abs(enc.y));
//    if (n.z < 0.0f)
//    {
//        n.xy = (1.0f - abs(n.yx)) * float2(n.x >= 0.0f ? 1.0f : -1.0f, n.y >= 0.0f ? 1.0f : -1.0f);
//    }
//    return normalize(n);
//}
//
//void Unpack(
//    uint4 gBuffer0,
//    uint4 gBuffer1,
//    out float3 normal,
//    out float3 position,
//    out float2 uv,
//    out float3 albedo,
//    out float metallic,
//    out float roughness,
//    out float3 motionVector,
//    out uint instanceID
//){
//    //uint2 unpackedNormal = uint2((gBuffer0.x & 0xFFFF0000) >> 16, gBuffer0.x & 0xFFFF);
//    uint2 unpackedOcta = uint2((gBuffer0.x & 0xFFFF0000) >> 16, gBuffer0.x & 0xFFFF);
//    uint3 unpackedPosition = uint3(gBuffer0.y & 0xFFFF, (gBuffer0.z & 0xFFFF0000) >> 16, gBuffer0.z & 0xFFFF);
//    uint2 unpackedUV = uint2((gBuffer0.w & 0xFFFF0000) >> 16, gBuffer0.w & 0xFFFF);
//
//    uint3 unpackedAlbedo = uint3((gBuffer1.x & 0xFFFF0000) >> 16, gBuffer1.x & 0xFFFF, (gBuffer1.y & 0xFFFF0000) >> 16);
//    uint unpackedMetallic = gBuffer1.y & 0xFFFF;
//    uint unpackedRoughness = (gBuffer1.z & 0xFFFF0000) >> 16;
//    uint3 unpackedMotionVector = uint3(gBuffer1.z & 0xFFFF, (gBuffer1.w & 0xFFFF0000) >> 16, gBuffer1.w & 0xFFFF);
//
//    float2 octa = f16tof32(unpackedOcta);
//    normal = DecodeNormalOctahedron(octa);
//    //normal.xy = f16tof32(unpackedNormal);
//    //normal.z = sqrt(1.f - dot(normal.xy, normal.xy));
//    position = f16tof32(unpackedPosition);
//    uv = f16tof32(unpackedUV);
//    albedo = f16tof32(unpackedAlbedo);
//    metallic = f16tof32(unpackedMetallic);
//    roughness = f16tof32(unpackedRoughness);
//    motionVector = f16tof32(unpackedMotionVector);
//    instanceID = (gBuffer0.y & 0xFFFF0000) >> 16;
//}


PSOutput main(PSInput input)
{
    PSOutput output;

    uint3 coord = uint3(uint2(input.texCoord * float2(screenBuffer.WindowRes.x, screenBuffer.WindowRes.y)), 0);
    uint4 gBufferValue0 = gBuffer0.Load(coord);
    uint4 gBufferValue1 = gBuffer1.Load(coord);
    float3 fragNormal;
    float3 fragPos;
    float2 fragUV;
    float3 fragAlbedo;
    float metallic;
    float roughness;
    float3 motionVector;
    uint instanceID;

    UnpackGbuffer(
        gBufferValue0, 
        gBufferValue1, 
        fragNormal, 
        fragPos, 
        fragUV, 
        fragAlbedo, 
        metallic, 
        roughness,
        motionVector,
        instanceID);

    //float depth = input.depth;

    float3 fragcolor = float3(0.f, 0.f, 0.f);

    for (int i = 0; i < lightBuffer.lightNum; i++)
    {
        float ocllusion = PCF(
            shadowMaps[i],
            linearSampler,
            lightBuffer.lights[i],
            fragPos
            );

        //float3 N = dot(fragNormal, )

        float3 L = normalize(-lightBuffer.lights[i].direction);
        float3 V = normalize(cameraBuffer.cameraPos.xyz - fragPos);
        //float3 N = dot(fragNormal, V) < 0.f ? fragNormal.xyz : float3(fragNormal.xy, -fragNormal.z);
        float3 R = reflect(-V, fragNormal);

        fragcolor += (1.f - ocllusion) * BRDF(fragAlbedo.rgb, lightBuffer.lights[i].intensity * lightBuffer.lights[i].color, L, V, fragNormal, metallic, roughness);
    }
    fragcolor += fragAlbedo.rgb * 0.04f; //ambient

    fragcolor.rgb = clamp(fragcolor.rgb, 0.0, 1.0);

    //sponza scene
    if(instanceID==46){
        output.color = float4(0.f, 0.f, 0.f, 1.f);
        return output;
    }

    if(outputContent==OutputContent::Render){
        output.color = float4(fragcolor, 1.f);
        //output.color = float4(motionVector.xy, 0.f, 1.f);// * 100.f;
    }
    else if(outputContent==OutputContent::Depth){
        //output.color = float4(depth * float3(1.f, 1.f, 1.f), 1.f);
        output.color = float4(fragcolor, 1.f);
    }
    else if(outputContent==OutputContent::Albedo){
        output.color = float4(fragAlbedo, 1.f);
    }
    else if(outputContent==OutputContent::Normal){
        output.color = float4(fragNormal * 2.f - 1.f, 1.f);
    }
    else if(outputContent==OutputContent::Position){
        output.color = float4(fragPos, 1.f);
    }
    else if(outputContent==OutputContent::MotionVector){
        output.color = float4(motionVector.xy * 10.f, 0.f, 1.f);
    }
    else if(outputContent==OutputContent::Roughness){
        output.color = float4(roughness, roughness, roughness, 1.f);
    }
    else if(outputContent==OutputContent::Metallic){
        output.color = float4(metallic, metallic, metallic, 1.f);
    }
    else if(outputContent==OutputContent::ShadowMap){
        float depth = shadowMaps[0].Sample(linearSampler, input.texCoord);
        output.color = float4(depth * float3(1.f, 1.f, 1.f), 1.f);
    }
    else{
        output.color = float4(0.f, 0.f, 0.f, 1.f);
    }
    
    return output;
}
