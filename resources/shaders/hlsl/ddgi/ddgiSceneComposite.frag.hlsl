#include "Common.h"

[[vk::binding(0, 0)]]
cbuffer ddgiCompositeBuffer : register(b0) 
{
    DDGICompositeBuffer ubo0; 
};

[[vk::binding(1, 0)]]Texture2D<float4>   DirectLight : register(t0);
[[vk::binding(2, 0)]]Texture2D<float4>   IndirectLight : register(t1);
[[vk::binding(3, 0)]]Texture2D<float>    RTAO : register(t2);
[[vk::binding(4, 0)]]SamplerState        linearSampler : register(s0);

struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    float3 directLight = DirectLight.Sample(linearSampler, input.texCoord).rgb;
    float3 indirectLight = IndirectLight.Sample(linearSampler, input.texCoord).rgb;
    float rtao = RTAO.Sample(linearSampler, input.texCoord).r;

    float3 finalColor;

    if (ubo0.visulizeMode == VisulizeDDGI_DI)
        finalColor = directLight;
    else if (ubo0.visulizeMode == VisulizeDDGI_GI)
        finalColor = indirectLight;
    else if (ubo0.visulizeMode == VisulizeDDGI_AO)
        finalColor = float3(rtao, rtao, rtao);
    else if (ubo0.visulizeMode == VisulizeDDGI_NOAO)
        finalColor = directLight + indirectLight;
    else
        finalColor = rtao * (directLight + indirectLight);

    output.color = float4(finalColor, 1.f);
    
    return output;
}
