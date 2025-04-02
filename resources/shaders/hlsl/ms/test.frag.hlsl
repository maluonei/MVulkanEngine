struct MeshOutput {
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET0;
};

PSOutput main(MeshOutput input)
{
    PSOutput output;

    output.Color = float4(input.Color, 1.0f);
    return output;
}