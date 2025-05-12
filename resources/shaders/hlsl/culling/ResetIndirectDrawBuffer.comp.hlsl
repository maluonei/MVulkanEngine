[[vk::binding(0, 0)]] RWStructuredBuffer<int> NumDrawInstances : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    NumDrawInstances[0] = 0;
}