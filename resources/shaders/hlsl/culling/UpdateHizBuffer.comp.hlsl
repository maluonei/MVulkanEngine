#include "Common.h"

[[vk::binding(0, 0)]] StructuredBuffer<HizDimension> HizDimensions : register(t0); 
[[vk::binding(1, 0)]] RWStructuredBuffer<HIZBuffer> hizBuffers : register(u1); 

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID){
    int prevLevel = hizBuffers[0].u_previousLevel;

    hizBuffers[0].u_previousLevel = prevLevel + 1;
    hizBuffers[0].u_previousLevelDimensions = HizDimensions[prevLevel + 1].u_previousLevelDimensions;
}