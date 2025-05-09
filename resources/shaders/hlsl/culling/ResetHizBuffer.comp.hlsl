#include "Common.h"

[[vk::binding(0, 0)]] RWStructuredBuffer<HIZBuffer> hizBuffers : register(u0); 

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID){
    hizBuffers[0].u_previousLevel = -1;
}