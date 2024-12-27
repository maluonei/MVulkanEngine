// 常量值，例如数组的宽度
[[vk::binding(0, 0)]]cbuffer Constants : register(b0)
{
    uint Width; // 输入数据的宽度
    uint Height; // 输入数据的宽度
}

// 输入/输出的缓冲区资源
[[vk::binding(1, 0)]] StructuredBuffer<float> InputBuffer : register(u0);  // 输入缓冲区
[[vk::binding(2, 0)]] RWStructuredBuffer<float> OutputBuffer : register(u1); // 输出缓冲区
[[vk::binding(3, 0)]] Texture2D<float4> InputTexture : register(t0);  // 输入缓冲区
[[vk::binding(4, 0)]] RWTexture2D<float4> OutputTexture : register(t1); // 输出缓冲区

[[vk::binding(5, 0)]] SamplerState linearSampler : register(s0);

// 定义线程组的大小
[numthreads(16, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    float2 texCoord = float2(threadID.x/(float)(Width), threadID.y/(float)(Height));
    //float2 texCoord = float2(threadID.x/256.f, threadID.y/256.f);

    // 从 UAV（Unordered Access View）读取输入
    float value1 = InputBuffer[threadID.x + threadID.y * Width];
    float3 value2 = InputTexture.Sample(linearSampler, texCoord).rgb;

    // 执行计算
    float result1 = value1 * value1;
    //float result2 = value2;

    // 将结果写回到 UAV
    OutputBuffer[threadID.x + threadID.y * Width] = result1;
    OutputTexture[float2(threadID.x, threadID.y)] = float4(value2, 1.f); // 使用索引直接写入
}
