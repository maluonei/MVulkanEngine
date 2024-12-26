struct PSInput
{
    float4 position : SV_POSITION;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    output.Color = float4(1.f, 0.f, 0.f, 1.f);

    return output;
}