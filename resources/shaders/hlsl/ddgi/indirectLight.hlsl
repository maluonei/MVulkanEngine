#include "Octahedral.hlsl"

struct Probe{
    float3 position;
    int padding0;
};

struct UniformBuffer1
{
    Probe probes[512];
};

int GetProbeIndex(uint3 pIndex){
    return pIndex.x * 64 + pIndex.y * 8 + pIndex.z;
}

float TrilinearInterpolationWeight(float3 offset, int base){
    float3 coefficients =  (float3(1.f, 1.f, 1.f) - base) * offset;
    return coefficients.x * coefficients.y * coefficients.z;
}

float DirectionWeight(float3 normal, float3 probePosition, float3 position){
    float3 direction = normalize(probePosition - position);
    float cosTheta = dot(direction, normal);

    return 0.2f + pow((cosTheta+1.) / 2., 2);

}

float ChebyshevTest(float depth, float depthSquared, float distance){
    if(distance < depth){
        return 1.f;
    }
    
    float miu = depth;
    float sigma = depthSquared - depth * depth;
    float sigma2 = sigma * sigma;
    float d_minus_miu = distance - miu;

    float p = sigma2 / (sigma2 + d_minus_miu * d_minus_miu);

    return pow(p, 3);
}

float3 CalculateIndirectLighting(
    UniformBuffer1 ubo1,
    RWTexture2D<float4> VolumeProbeDatasRadiance,
    RWTexture2D<float4> VolumeProbeDatasDepth,
    float3 probePos0,
    float3 probePos1,
    float3 fragPos, 
    float3 fragNormal){
    float3 probeOffset = (fragPos - probePos0) / (probePos1 - probePos0);
    uint3 probeOffsetInt = uint3(probeOffset);

    int probeIndex = GetProbeIndex(probeOffsetInt);

    float totalWeights = 0.f;
    float3 totalRadiance = float3(0.f, 0.f, 0.f);

    for(int x=0;x<2;x++){
        for(int y=0;y<2;y++){
            for(int z=0;z<2;z++){
                int3 probeBaseIndex = probeOffsetInt + int3(x, y, z);
                int pIndex = GetProbeIndex(probeOffsetInt + int3(x, y, z));

                float3 probePosition = ubo1.probes[pIndex].position;
                float2 octahedralUV = DDGIGetOctahedralCoordinates(fragNormal);
                uint2 probeBaseUV = uint2(8, 8) * uint2(probeBaseIndex.x * 8 + probeBaseIndex.y, probeBaseIndex.z);
                uint2 octahedralUVinTexture_Int = probeBaseUV + octahedralUV * uint2(6, 6) + uint2(1, 1);
                float2 octahedralUVinTexture_Float = octahedralUVinTexture_Int / float2(512., 64.);
                
                float3 radiance = VolumeProbeDatasRadiance[octahedralUVinTexture_Float].rgb;
                float2 depths = VolumeProbeDatasDepth[octahedralUVinTexture_Float].rg;
                float depth = depths.x;
                float depthSquared = depths.y;

                float distance = length(probePosition - fragPos);
                
                float trilinearWeight = TrilinearInterpolationWeight(probeOffset - probeOffsetInt, float3(x, y, z));
                float directionWeight = DirectionWeight(fragNormal, probePosition, fragPos);
                float chebyshevWeight = ChebyshevTest(depth, depthSquared, distance);

                float combinedWeight = trilinearWeight * directionWeight * chebyshevWeight;

                totalRadiance += radiance * combinedWeight;
                totalWeights += combinedWeight;
            }
        }   
    }

    totalRadiance /= totalWeights;

    return totalRadiance;
}