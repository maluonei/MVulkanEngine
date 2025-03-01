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

float TrilinearInterpolationWeight(uint3 base, float3 offset){
    float3 coefficients =  saturate(abs((float3(1.f, 1.f, 1.f) - base) - offset));
    return coefficients.x * coefficients.y * coefficients.z;
}

float DirectionWeight(float3 normal, float3 probePosition, float3 position){
    float3 direction = normalize(probePosition - position);
    float cosTheta = max(dot(direction, normal), 0.f);

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
    Texture2D<float4> VolumeProbeDatasRadiance,
    Texture2D<float4> VolumeProbeDatasDepth,
    SamplerState linearSampler,
    float3 probePos0,
    float3 probePos1,
    float3 fragPos, 
    float3 fragNormal,
    float3 fragAlbedo){
    
    float3 probeOffset = (fragPos - probePos0) / (probePos1 - probePos0);
    uint3 probeOffsetInt = uint3(probeOffset * 7);

    //int probeIndex = GetProbeIndex(probeOffsetInt);

    float totalWeights = 0.f;
    float3 totalRadiance = float3(0.f, 0.f, 0.f);

    float2 octahedralUVOfNormal = DDGIGetOctahedralCoordinates(fragNormal);
    //octahedralUVOfNormal = (octahedralUVOfNormal + 1.f) / 2.f;

    for(int x=0;x<2;x++){
        for(int y=0;y<2;y++){
            for(int z=0;z<2;z++){
                int3 probeBaseIndex = probeOffsetInt + int3(x, y, z);
                if(probeBaseIndex.x<0 || probeBaseIndex.x>=8 || probeBaseIndex.y<0 || probeBaseIndex.y>=8 || probeBaseIndex.z<0 || probeBaseIndex.z>=8){
                    continue;
                }

                int pIndex = GetProbeIndex(probeBaseIndex);

                float3 probePosition = ubo1.probes[pIndex].position;
                float3 direction = normalize(fragPos - probePosition);
                
                uint2  probeBaseUV = uint2(8, 8) * uint2(probeBaseIndex.x * 8 + probeBaseIndex.y, probeBaseIndex.z) + uint2(4, 4);

                float2 octahedralUVOfDirection = DDGIGetOctahedralCoordinates(direction);
                //octahedralUVOfDirection = (octahedralUVOfDirection + 1.f) / 2.f;
                uint2  octahedralUVOfDirectioninTexture_Int = probeBaseUV + octahedralUVOfDirection * uint2(3, 3);
                float2 octahedralUVOfDirectioninTexture_Float = octahedralUVOfDirectioninTexture_Int / float2(512., 64.);
                
                uint2  octahedralUVOfNormalTexture_Int = probeBaseUV + octahedralUVOfNormal * uint2(3, 3);
                float2 octahedralUVOfNormalTexture_Float = octahedralUVOfDirectioninTexture_Float / float2(512., 64.);
                
                //float3 radiance = VolumeProbeDatasRadiance[octahedralUVOfNormalTexture_Float].rgb;
                //float2 depths = VolumeProbeDatasDepth[octahedralUVOfDirectioninTexture_Float].rg;
                
                float3 radiance = VolumeProbeDatasRadiance.Sample(linearSampler, octahedralUVOfNormalTexture_Float).rgb;
                float2 depths = VolumeProbeDatasDepth.Sample(linearSampler, octahedralUVOfDirectioninTexture_Float).rg;
                
                float  depth = depths.x;
                float  depthSquared = depths.y;

                float distance = length(probePosition - fragPos);
                
                float trilinearWeight = TrilinearInterpolationWeight(probeOffset - probeOffsetInt, float3(x, y, z));
                float directionWeight = DirectionWeight(fragNormal, probePosition, fragPos);
                float chebyshevWeight = ChebyshevTest(depth, depthSquared, distance);

                //trilinearWeight = 1.f;
                //return float3(trilinearWeight, directionWeight, chebyshevWeight);

                float combinedWeight = trilinearWeight * directionWeight * chebyshevWeight;

                totalRadiance += radiance * combinedWeight;
                totalWeights += combinedWeight;
            }
        }   
    }

    totalRadiance /= max(totalWeights, 1e-8);

    totalRadiance = totalRadiance * fragAlbedo.rgb / PI;

    //totalRadiance = float3(totalWeights, totalWeights, totalWeights);

    return totalRadiance;
}