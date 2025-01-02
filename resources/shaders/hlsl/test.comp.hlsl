[[vk::binding(0, 0)]]cbuffer Constants : register(b0)
{
    uint Width; // �������ݵĿ���
    uint Height; // �������ݵĿ���
}

[[vk::binding(1, 0)]] StructuredBuffer<float> InputBuffer : register(t0); 
[[vk::binding(2, 0)]] RWStructuredBuffer<float> OutputBuffer : register(u0); 
[[vk::binding(3, 0)]] Texture2D<float4> InputTexture : register(t1);  
[[vk::binding(4, 0)]] RWTexture2D<float4> OutputTexture : register(u1);

[[vk::binding(5, 0)]] SamplerState linearSampler : register(s0);


[numthreads(16, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    float2 texCoord = float2(threadID.x/(float)(Width), threadID.y/(float)(Height));
   
    float value1 = InputBuffer[threadID.x + threadID.y * Width];
    float3 value2 = InputTexture.Sample(linearSampler, texCoord).rgb;

    float result1 = value1 * value1;

    OutputBuffer[threadID.x + threadID.y * Width] = result1;
    OutputTexture[float2(threadID.x, threadID.y)] = float4(value2, 1.f);
}
