struct MeshOutput {
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float3 Color    : COLOR;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET0;
};

PSOutput main(MeshOutput input)
{
    PSOutput output;

    float3 normal = normalize(input.Normal);

    //output.Color = float4(input.Color, 1.0f);
    output.Color = float4(normal * 2.f - 1.f, 1.0f);
    return output;
}