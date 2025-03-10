#include "indirectLight.hlsli"

struct UniformBuffer0{
    float sharpness;
    int padding0;
    int padding1;
    int padding2;
};

[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UniformBuffer0 ubo0;
}; 

[[vk::binding(1, 0)]]
cbuffer ub1 : register(b1)
{
    UniformBuffer1 ubo1;
};

[[vk::binding(2, 0)]]StructuredBuffer<Probe> probes : register(t5);

[numthreads(16, 1, 1)] 
void main(uint3 DispatchThreadID : SV_DispatchThreadID){
    uint index = DispatchThreadID.x;
    for(int i=0;i<ubo1.raysPerProbe;i++){
        if()
    }
}