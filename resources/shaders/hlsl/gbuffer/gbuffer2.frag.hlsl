//#include "Common.h"
#include "util.hlsli"

[[vk::binding(1, 0)]]
cbuffer vpBuffer_p : register(b1)
{
    VPBuffer vp_p;
};

//[[vk::binding(3, 0)]]
//cbuffer texBuffer : register(b2)
//{
//    TexBuffer texbuffer;
//}; 

[[vk::binding(6, 0)]]
cbuffer gBufferInfoBuffer : register(b3)
{
    MScreenBuffer gBufferInfoBuffer;
}; 

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
    //float4 positionPreviousFrame : TEXCOORD3;
    //float3x3 TBN : TEXCOORD1;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
};

struct PSOutput
{
    [[vk::location(0)]] uint4 gBuffer0 : SV_TARGET0; // 16 bytes normalx, 16bytes normaly, 16bytes normalz, 16bytes positionx, 16bytes positiony, 16bytes positionz, 16bytes u, 16bytes v
    [[vk::location(1)]] uint4 gBuffer1 : SV_TARGET1; // 16 bytes albedo r, 16bytes albedo g, 16bytes albedo b, 16bytes metallic, 16bytes roughness
};

PSOutput main(PSInput input)
{
    PSOutput output;

    //float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));

    int matId = materialIds[input.instanceID];

    int diffuseTextureIdx = materials[matId].diffuseTextureIdx;
    int metallicAndRoughnessTextureIdx = materials[matId].metallicAndRoughnessTextureIdx;
    //int normalTextureIdx = materials[matId].normalTextureIdx;


    //int diffuseTextureIdx = texbuffer.tex[input.instanceID].diffuseTextureIdx;
    //int metallicAndRoughnessTextureIdx = texbuffer.tex[input.instanceID].metallicAndRoughnessTextureIdx;
    //int normalTextureIdx = texbuffer.tex[input.instanceID].normalTextureIdx;

    float3 normal = normalize(input.normal);
    float3 position = input.worldPos;
    float2 uv = input.texCoord;
    float3 albedo;
    float3 metallicAndRoughness;
    
    float4 screenSpacePositionPrevFrame = mul(vp_p.View, float4(input.worldPos, 1.f));
    screenSpacePositionPrevFrame = mul(vp_p.Projection, screenSpacePositionPrevFrame);
    screenSpacePositionPrevFrame /= screenSpacePositionPrevFrame.w;
    screenSpacePositionPrevFrame.xy = float2(
        (screenSpacePositionPrevFrame.x + 1.0) * 0.5,
        (1.0 - screenSpacePositionPrevFrame.y) * 0.5  // Y轴翻转
    );
    //output.positionPreviousFrame = screenSpacePositionPrevFrame;

    float3 motionVector = (input.position.xyz / float3(gBufferInfoBuffer.WindowRes, 1.f) - screenSpacePositionPrevFrame.xyz);

    // Sample diffuse texture if valid, otherwise default to white
    if (diffuseTextureIdx != -1)
    {
        albedo = textures[diffuseTextureIdx].Sample(linearSampler, input.texCoord).rgb;
    }
    else
    {
        //output.Albedo = float4(1.0, 1.0, 1.0, 1.0);
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

    //motionVector = input.position.xyz - input.positionPreviousFrame.xyz;

    PackGbuffer(
        normal, 
        position, 
        uv, 
        albedo, 
        metallicAndRoughness, 
        motionVector, 
        input.instanceID,
        output.gBuffer0, 
        output.gBuffer1);


    return output;
}