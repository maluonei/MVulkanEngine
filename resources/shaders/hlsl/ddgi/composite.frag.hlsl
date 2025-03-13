struct UniformBuffer0{
    int visulizeProbe;
    int padding0;
    int padding1;
    int padding2;
};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0) 
{
    UniformBuffer0 ubo0; 
};


[[vk::binding(1, 0)]]Texture2D<float4>   DirectLight : register(t0);
[[vk::binding(2, 0)]]Texture2D<float4>   IndirectLight : register(t1);
[[vk::binding(3, 0)]]Texture2D<float4>   RTAO : register(t2);
[[vk::binding(4, 0)]]Texture2D<float4>   ProbeVisulize : register(t3);
[[vk::binding(5, 0)]]SamplerState        linearSampler : register(s0);

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
    float4 gBufferValue3 = ProbeVisulize.Sample(linearSampler, input.texCoord);
 
    float3 directLight = gBufferValue0.rgb;
    float3 indirectLight = gBufferValue1.rgb;
    float rtao = gBufferValue2.r;
    float3 probeVisulize = gBufferValue3.rgb;
    //float hasProbe = gBufferValue3.a;

    bool hasProbe = (gBufferValue3.a - 0.5f) < 1e-8;
    float3 finalColor;
    if(ubo0.visulizeProbe != 0)
        finalColor = rtao * (directLight + indirectLight) * (1-hasProbe) + hasProbe * probeVisulize;
    else{
        finalColor = rtao * (directLight + indirectLight);
    }
    //float3 finalColor = rtao * (directLight + indirectLight) + (1e-20) * hasProbe * probeVisulize;

    output.color = float4(finalColor, 1.f);

    //output.color = float4(1.f, 0.f, 0.f, 1.f);
    
    return output;
}
