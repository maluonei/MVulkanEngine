// HLSL Shader
//#define SHARED_CODE_HLSL

#include "util.hlsli"

//struct MVPBuffer
//{
//    float4x4 Model;
//    float4x4 View;
//    float4x4 Projection;
//};


[[vk::binding(0, 0)]]
cbuffer vpBuffer : register(b0)
{
    VPBuffer vp;
};


[[vk::binding(2, 0)]] StructuredBuffer<ModelBuffer> ModelBuffers : register(t0);
[[vk::binding(8, 0)]] StructuredBuffer<int> CulledIndirectInstances : register(t10);
[[vk::binding(9, 0)]] StructuredBuffer<float4> InstanceDebugDepth : register(t11);

//#define MVPBuffer ubo0
//[[vk::binding(2, 0)]] Texture2D textures[1024] : register(t2);

struct VSInput
{
    [[vk::location(0)]] float3 Position : POSITION;
    [[vk::location(1)]] float3 Normal : NORMAL;
    [[vk::location(2)]] float2 Coord : TEXCOORD0;
    [[vk::location(3)]] float3 Tangent : TEXCOORD1;
    [[vk::location(4)]] float3 Bitangent : TEXCOORD2;
    uint InstanceID : SV_InstanceID;
};

struct VSOutput
{
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;
    //float4 positionPreviousFrame : TEXCOORD3;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float2 depth : TEXCOORD3;

    //float3x3 TBN : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    int instanceIndex = CulledIndirectInstances[input.InstanceID];

    output.depth = InstanceDebugDepth[instanceIndex];

    float4x4 Model = ModelBuffers[instanceIndex].Model;

    // Pass through texture coordinates
    output.texCoord = input.Coord;

    // Compute world space position
    float4 worldSpacePosition = mul(Model, float4(input.Position, 1.0));
    //float4 worldSpacePositionPrevFrame = mul(Model, float4(input.Position, 1.0));

    // Calculate world position (divide by w to handle perspective divide)
    output.worldPos = worldSpacePosition.xyz / worldSpacePosition.w;
    //output.worldPosPrevFrame = worldSpacePositionPrevFrame.xyz / worldSpacePositionPrevFrame.w;
    // Compute screen space position
    float4 screenSpacePosition = mul(vp.View, worldSpacePosition);
    screenSpacePosition = mul(vp.Projection, screenSpacePosition);
    output.position = screenSpacePosition;

    //float4 screenSpacePositionPrevFrame = mul(vp_p.View, worldSpacePosition);
    //screenSpacePositionPrevFrame = mul(vp_p.Projection, screenSpacePositionPrevFrame);
    ////output.positionPreviousFrame = screenSpacePositionPrevFrame;
    //output.positionPreviousFrame = screenSpacePosition;
    //
    //output.positionPreviousFrame.xyzw /= output.positionPreviousFrame.w;
    //// Vulkan NDC到UV空间转换（Y轴需要翻转）
    //output.positionPreviousFrame.xy = float2(
    //    (output.positionPreviousFrame.x + 1.0) * 0.5,
    //    (1.0 - output.positionPreviousFrame.y) * 0.5  // Y轴翻转
    //);
    //output.positionPreviousFrame.xyzw /= output.positionPreviousFrame.w;
    //[-1, 1] -> [0, 1]
    //output.positionPreviousFrame.xyz = (screenSpacePositionPrevFrame.xyz + 1.f) * 0.5f;

    // Compute normal in world space
    float3x3 normalMatrix = transpose(inverse((float3x3)Model));
    output.normal = normalize(mul(normalMatrix, input.Normal));

    // Pass instance ID
    output.instanceID = instanceIndex;

    //float3 calculatedNormal = cross(input.Tangent, input.Bitangent);
    //if(dot(calculatedNormal, input.Normal) < 0.0)
    //    input.Tangent *= -1.f;

    float4 tangent = mul(Model, float4(input.Tangent, 1.0));
    //float4 bitangent = mul(vp.Model, float4(input.Bitangent, 1.0));
    output.tangent = normalize(tangent.xyz / tangent.w);
    output.bitangent = cross(output.normal, output.tangent);

    return output;
}