// ����ֵ����������Ŀ���
[[vk::binding(0, 0)]]cbuffer Constants : register(b0)
{
    uint Width; // �������ݵĿ���
    uint Height; // �������ݵĿ���
}

// ����/����Ļ�������Դ
[[vk::binding(1, 0)]] StructuredBuffer<float> InputBuffer : register(u0);  // ���뻺����
[[vk::binding(2, 0)]] RWStructuredBuffer<float> OutputBuffer : register(u1); // ���������
[[vk::binding(3, 0)]] Texture2D<float4> InputTexture : register(t0);  // ���뻺����
[[vk::binding(4, 0)]] RWTexture2D<float4> OutputTexture : register(t1); // ���������

[[vk::binding(5, 0)]] SamplerState linearSampler : register(s0);



// �����߳���Ĵ�С
[numthreads(16, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    float2 texCoord = float2(threadID.x/16.f, threadID.y/16.f);

    // �� UAV��Unordered Access View����ȡ����
    float value1 = InputBuffer[threadID.x + threadID.y * Width];
    float value2 = InputTexture.Sample(linearSampler, texCoord);

    // ִ�м���
    float result1 = value1 * value1;
    float result2 = value2 * value2;

    // �����д�ص� UAV
    OutputBuffer[threadID.x + threadID.y * Width] = result1;
    OutputTexture[texCoord] = float4(result2); // ʹ������ֱ��д��
}