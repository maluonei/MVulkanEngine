#include "Octahedral.hlsl"

[[vk::binding(2, 0)]] Texture2D<float4> VolumeProbeDatasRadiance  : register(t0); 
[[vk::binding(3, 0)]] SamplerState linearSampler : register(s0);

struct PSInput
{
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    [[vk::location(0)]] float4 color : SV_TARGET0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    int probeIndex = input.instanceID;
    int2 probeBaseIndex = int2(probeIndex / 8, probeIndex % 8);

    float2 octahedralUV = DDGIGetOctahedralCoordinates(input.normal);
    //octahedralUV = (octahedralUV + 1.f) / 2.f;
    float2 octahedralUVInTextureFloat = (probeBaseIndex * float2(8, 8) + float2(4, 4) + float2(3, 3) * octahedralUV) / float2(512, 64);
    
    float3 probeRadiance = VolumeProbeDatasRadiance.Sample(linearSampler, octahedralUVInTextureFloat).rgb;

    output.color = float4(probeRadiance, 0.5);
    //output.color = float4(input.normal + 0.000000001f*probeRadiance, 0.5);
    return output;
}