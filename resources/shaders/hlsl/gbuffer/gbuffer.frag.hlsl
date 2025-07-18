//#include "Common.h"
#include "util.hlsli"

//[[vk::binding(6, 0)]]
//cbuffer gBufferInfoBuffer : register(b3)
//{
//    MScreenBuffer gBufferInfoBuffer;
//}; 

[[vk::binding(3, 0)]] StructuredBuffer<MaterialBuffer> materials : register(t11); 
[[vk::binding(7, 0)]] StructuredBuffer<int> materialIds : register(t12); 

[[vk::binding(4, 0)]] Texture2D textures[MAX_TEXTURES] : register(t2);
[[vk::binding(5, 0)]] SamplerState linearSampler : register(s2);

struct PSInput
{
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;

    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
};

struct PSOutput
{
    //[[vk::location(0)]] uint4 gBuffer0 : SV_TARGET0; // 16 bytes normalx, 16bytes normaly, 16bytes normalz, 16bytes positionx, 16bytes positiony, 16bytes positionz, 16bytes u, 16bytes v
    //[[vk::location(1)]] uint4 gBuffer1 : SV_TARGET1; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
    //[[vk::location(2)]] float4 gBuffer2 : SV_TARGET2; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
    //[[vk::location(3)]] float4 gBuffer3 : SV_TARGET3; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
    //[[vk::location(4)]] float4 gBuffer4 : SV_TARGET4; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
    //[[vk::location(5)]] float4 gBuffer5 : SV_TARGET5; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
    [[vk::location(0)]] float4 gBuffer0 : SV_TARGET0; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
    [[vk::location(1)]] float4 gBuffer1 : SV_TARGET1; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
    [[vk::location(2)]] float4 gBuffer2 : SV_TARGET2; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
    [[vk::location(3)]] float4 gBuffer3 : SV_TARGET3; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
};


PSOutput main(PSInput input)
{
    PSOutput output;

    int matId = materialIds[input.instanceID];

    int diffuseTextureIdx = materials[matId].diffuseTextureIdx;
    int metallicAndRoughnessTextureIdx = materials[matId].metallicAndRoughnessTextureIdx;

    float3 normal = normalize(input.normal);
    float3 position = input.worldPos;
    float2 uv = input.texCoord;
    float3 albedo;
    float3 metallicAndRoughness;
    
    //float4 screenSpacePositionPrevFrame = mul(vp_p.View, float4(input.worldPos, 1.f));
    //screenSpacePositionPrevFrame = mul(vp_p.Projection, screenSpacePositionPrevFrame);
    //screenSpacePositionPrevFrame /= screenSpacePositionPrevFrame.w;
    //screenSpacePositionPrevFrame.xy = float2(
    //    (screenSpacePositionPrevFrame.x + 1.0) * 0.5,
    //    (1.0 - screenSpacePositionPrevFrame.y) * 0.5  // Y轴翻转
    //);

    // float3 motionVector = (input.position.xyz / float3(gBufferInfoBuffer.WindowRes, 1.f) - screenSpacePositionPrevFrame.xyz);
    float3 motionVector = float3(0.f, 0.f, 0.f);

    // Sample diffuse texture if valid, otherwise default to white
    if (diffuseTextureIdx != -1)
    {
        albedo = textures[diffuseTextureIdx].Sample(linearSampler, input.texCoord).rgb;
    }
    else
    {
        albedo = materials[matId].diffuseColor; // Use diffuse color from UBO
    }
    // Sample metallic and roughness texture if valid, otherwise default values
    if (metallicAndRoughnessTextureIdx != -1)
    {
        metallicAndRoughness.rgb = textures[metallicAndRoughnessTextureIdx].Sample(linearSampler, input.texCoord).rgb;
    }
    else
    {
        metallicAndRoughness.rgb = float3(0.0, 0.5, 0.5);
    }

    //PackGbuffer(
    //    normal, 
    //    position, 
    //    uv, 
    //    albedo, 
    //    metallicAndRoughness, 
    //    motionVector, 
    //    input.instanceID,
    //    output.gBuffer0, 
    //    output.gBuffer1);

    output.gBuffer0.xyzw = float4(normal.xyz, 1.f);
    output.gBuffer1.xyzw = float4(position.xyz, 1.f);
    output.gBuffer2.xyzw = float4(uv, 0.f, 1.f);
    output.gBuffer3.xyzw = float4(albedo, 1.f);

    return output;
}