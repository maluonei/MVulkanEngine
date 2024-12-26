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


VSOutput main(VSInput input)
{
    VSOutput output;

    output.position = float4(input.Position, 1.0);

    return output;
}