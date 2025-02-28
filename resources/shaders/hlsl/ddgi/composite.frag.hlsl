[[vk::binding(0, 0)]]Texture2D<float4>   DirectLight : register(t0);
[[vk::binding(1, 0)]]Texture2D<float4>   IndirectLight : register(t1);
[[vk::binding(2, 0)]]Texture2D<float4>   RTAO : register(t2);
[[vk::binding(3, 0)]]SamplerState        linearSampler : register(s0);

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
    
    float4 gBufferValue0 = DirectLight.Sample(linearSampler, input.texCoord);
    float4 gBufferValue1 = IndirectLight.Sample(linearSampler, input.texCoord);
    float4 gBufferValue2 = RTAO.Sample(linearSampler, input.texCoord);

    float3 directLight = gBufferValue0.rgb;
    float3 indirectLight = gBufferValue1.rgb;
    float rtao = gBufferValue2.r;

    float3 finalColor = rtao * (directLight + indirectLight);

    output.color = float4(finalColor, 1.f);
    
    return output;
}
