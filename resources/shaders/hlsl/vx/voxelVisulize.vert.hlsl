struct VSInput
{
    [[vk::location(0)]] float3 Position : POSITION;
    [[vk::location(1)]] float3 Normal : NORMAL;
    [[vk::location(2)]] float2 Coord : TEXCOORD0;
    uint InstanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

// HLSL Shader
struct ModelBuffer
{
    float4x4 Model;
};

struct VPBuffer{
    float4x4 View;
    float4x4 Projection;
};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    VPBuffer VP;
};

[[vk::binding(1, 0)]]StructuredBuffer<ModelBuffer> Models : register(t0);


VSOutput main(VSInput input)
{
    VSOutput output;

    //output.position = float4(input.Position, 1.0);
    output.position = mul(Models[input.InstanceID].Model, float4(input.Position, 1.0));
    output.position = mul(VP.View, output.position);
    output.position = mul(VP.Projection, output.position);

    return output;
}