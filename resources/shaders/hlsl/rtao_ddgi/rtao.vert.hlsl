// HLSL Shader
struct VSInput
{
    [[vk::location(0)]] float3 Position : POSITION;
    [[vk::location(1)]] float3 Normal : NORMAL;
    [[vk::location(2)]] float2 Coord : TEXCOORD0;
};

struct VSOutput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.texCoord = input.Coord;
    output.normal = input.Normal;
    output.position = float4(input.Position, 1.f);

    return output;
}