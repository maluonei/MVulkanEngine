#include "Octahedral.hlsl"

struct Probe{
    float3 position;
    int padding0;
};

struct UniformBuffer1
{
    Probe probes[512];
};

int GetProbeIndex(int3 pIndex, int3 probeScale){
    return pIndex.x * probeScale.y * probeScale.z + pIndex.y * probeScale.z + pIndex.z;
}

int GetProbeIndex(int3 pIndex){
    return pIndex.x * 64 + pIndex.y * 8 + pIndex.z;
}

float TrilinearInterpolationWeight(float3 gridSpaceDistance, int3 probeIndex, float3 probeSpacing){
    //float3 gridSpaceDistance = position - probePosition;
    float3 alpha = clamp((gridSpaceDistance / probeSpacing), float3(0.f, 0.f, 0.f), float3(1.f, 1.f, 1.f));

    float3 trilinear = max(0.001f, lerp(1.f - alpha, alpha, probeIndex));
    float  trilinearWeight = (trilinear.x * trilinear.y * trilinear.z);

    return trilinearWeight;
}

float DirectionWeight(float3 normal, float3 position, float3 probePosition){
    float3 direction = normalize(probePosition - position);
    //float cosTheta = max(dot(direction, normal), 0.f);
    float cosTheta = dot(direction, normal);

    float weight = (cosTheta+1.f) * 0.5f;
    return 0.2f + weight * weight;

}

float ChebyshevTest(float depth, float depthSquared, float distance){
    if(distance < depth){
        return 1.f;
    }
    
    //float average = depth;
    float variance = depthSquared - (depth * depth);
    //float variance = variance * variance;
    float v = distance - depth;

    float p = variance / (variance + v * v);

    return max(p * p * p, 0.05f);
}

//float3 CalculateIndirectLighting(
//    UniformBuffer1 ubo1,
//    Texture2D<float4> VolumeProbeDatasRadiance,
//    Texture2D<float4> VolumeProbeDatasDepth,
//    SamplerState linearSampler,
//    float3 probePos0,
//    float3 probePos1,
//    float3 fragPos, 
//    float3 fragNormal,
//    float3 fragAlbedo){
//    
//    float3 probeOffset = (fragPos - probePos0) / (probePos1 - probePos0);
//    float3 probeOffsetinVolumn = float3(probeOffset * 7);
//    int3 probeOffsetBase = int3(floor(probeOffsetinVolumn));
//
//    //int probeIndex = GetProbeIndex(probeOffsetInt);
//
//    float totalWeights = 0.f;
//    float3 totalRadiance = float3(0.f, 0.f, 0.f);
//
//    float2 octahedralUVOfNormal = DDGIGetOctahedralCoordinates(fragNormal);
//    //octahedralUVOfNormal = (octahedralUVOfNormal + 1.f) / 2.f;
//
//    for(int x=0;x<2;x++){
//        for(int y=0;y<2;y++){
//            for(int z=0;z<2;z++){
//                int3 probeBaseIndex = probeOffsetBase + int3(x, y, z);
//                if(probeBaseIndex.x<0 || probeBaseIndex.x>=8 || probeBaseIndex.y<0 || probeBaseIndex.y>=8 || probeBaseIndex.z<0 || probeBaseIndex.z>=8){
//                    continue;
//                }
//
//                int pIndex = GetProbeIndex(probeBaseIndex);
//
//                float3 probePosition = ubo1.probes[pIndex].position;
//                float3 direction = normalize(fragPos - probePosition);
//                
//                uint2  probeBaseUV = uint2(8, 8) * uint2(probeBaseIndex.x * 8 + probeBaseIndex.y, probeBaseIndex.z) + uint2(4, 4);
//
//                float2 octahedralUVOfDirection = DDGIGetOctahedralCoordinates(direction);
//                //octahedralUVOfDirection = (octahedralUVOfDirection + 1.f) / 2.f;
//                uint2  octahedralUVOfDirectioninTexture_Int = probeBaseUV + octahedralUVOfDirection * uint2(3, 3);
//                float2 octahedralUVOfDirectioninTexture_Float = octahedralUVOfDirectioninTexture_Int / float2(512., 64.);
//                
//                uint2  octahedralUVOfNormalTexture_Int = probeBaseUV + octahedralUVOfNormal * uint2(3, 3);
//                float2 octahedralUVOfNormalTexture_Float = octahedralUVOfNormalTexture_Int / float2(512., 64.);
//                
//                //float3 radiance = VolumeProbeDatasRadiance[octahedralUVOfNormalTexture_Float].rgb;
//                //float2 depths = VolumeProbeDatasDepth[octahedralUVOfDirectioninTexture_Float].rg;
//                
//                float3 radiance = VolumeProbeDatasRadiance.Sample(linearSampler, octahedralUVOfNormalTexture_Float).rgb;
//                float2 depths = VolumeProbeDatasDepth.Sample(linearSampler, octahedralUVOfDirectioninTexture_Float).rg;
//                
//                float  depth = depths.x;
//                float  depthSquared = depths.y;
//
//                float distance = length(probePosition - fragPos);
//                
//                float trilinearWeight = TrilinearInterpolationWeight(float3(x, y, z), probeOffsetinVolumn - probeOffsetBase);
//                float directionWeight = DirectionWeight(fragNormal, fragPos, probePosition);
//                float chebyshevWeight = ChebyshevTest(depth, depthSquared, distance);
//
//                float combinedWeight = trilinearWeight * directionWeight * chebyshevWeight;
//
//                totalRadiance += radiance * combinedWeight;
//                totalWeights += combinedWeight;
//            }
//        }   
//    }
//
//    totalRadiance /= max(totalWeights, 1e-10);
//
//    totalRadiance = totalRadiance * fragAlbedo.rgb / PI;
//
//    return totalRadiance;
//}

#define radianceProbeResolution 8
#define depthProbeResolution 16
#define surfaceBias 0.02f

struct IndirectLightingOutput{
    float3 radiance;
    float3 radianceProbeUV;
    float3 depthProbeUV;
    float3 probeRadiance;
};

IndirectLightingOutput CalculateIndirectLighting(
    UniformBuffer1 ubo1,
    Texture2D<float4> VolumeProbeDatasRadiance,
    Texture2D<float4> VolumeProbeDatasDepth,
    SamplerState linearSampler,
    float3 probePos0,
    float3 probePos1,
    float3 fragPos, 
    float3 fragNormal){
    
    //fragPos = fragPos + fragNormal * 0.02f;
    float3 biasedPos = fragPos + fragNormal * surfaceBias;
    float3 probeSpacing = (probePos1 - probePos0) / 7.f;
    //int3   baseProbeCoords = DDGIGetBaseProbeGridCoords(biasedWorldPosition, volume);

    float3 probeOffset = saturate((biasedPos - probePos0) / (probePos1 - probePos0));
    float3 probeOffsetinVolumn = float3(probeOffset * 7);
    int3   probeOffsetBase = int3(probeOffsetinVolumn);
    int3   baseProbeCoords = int3(probeOffsetBase);

    float2 radianceProbeResolutionInv = 1.f / float2(radianceProbeResolution * 64.f, radianceProbeResolution * 8.f);
    //int probeIndex = GetProbeIndex(probeOffsetInt);

    float totalWeights = 0.f;
    float3 totalRadiance = float3(0.f, 0.f, 0.f);
    float3 test = float3(0.f, 0.f, 0.f);

    float2 octahedralUVOfNormal = DDGIGetOctahedralCoordinates(fragNormal);

    int pBaseIndex = GetProbeIndex(probeOffsetBase);
    float3 probeBasePosition = ubo1.probes[pBaseIndex].position;
    
    //octahedralUVOfNormal = (octahedralUVOfNormal + 1.f) / 2.f;
    IndirectLightingOutput output;

    for(int x=0;x<2;x++){
        for(int y=0;y<2;y++){
            for(int z=0;z<2;z++){
    //for(int x=1;x>=0;x--){
    //    for(int y=1;y>=0;y--){
    //        for(int z=1;z>=0;z--){
                //int3 probeBaseIndex = probeOffsetBase + int3(x, y, z);
                int3 probeBaseIndex = clamp(baseProbeCoords + int3(x, y, z), int3(0, 0, 0), int3(8, 8, 8) - int3(1, 1, 1));
                //if(probeBaseIndex.x<0 || probeBaseIndex.x>=8 || probeBaseIndex.y<0 || probeBaseIndex.y>=8 || probeBaseIndex.z<0 || probeBaseIndex.z>=8){
                //    continue;
                //}

                int pIndex = GetProbeIndex(probeBaseIndex);

                float3 probePosition = ubo1.probes[pIndex].position;
                float3 direction = normalize(biasedPos - probePosition);
                
                uint2  radianceProbeBaseUV = uint2(radianceProbeResolution, radianceProbeResolution) * uint2(probeBaseIndex.x * 8 + probeBaseIndex.y, probeBaseIndex.z) + uint2(radianceProbeResolution * 0.5f, radianceProbeResolution * 0.5f);
                uint2  depthProbeBaseUV = uint2(depthProbeResolution, depthProbeResolution) * uint2(probeBaseIndex.x * 8 + probeBaseIndex.y, probeBaseIndex.z) + uint2(depthProbeResolution * 0.5f, depthProbeResolution * 0.5f);

                float2 octahedralUVOfDirection = DDGIGetOctahedralCoordinates(direction);
                //octahedralUVOfDirection = (octahedralUVOfDirection + 1.f) / 2.f;
                float2 octahedralUVOfDirectioninTexture_Int = float2(depthProbeBaseUV) + octahedralUVOfDirection * float2((depthProbeResolution-2) * 0.5f, (depthProbeResolution-2) * 0.5f);
                float2 octahedralUVOfDirectioninTexture_Float = float2(octahedralUVOfDirectioninTexture_Int) / float2(depthProbeResolution*64., depthProbeResolution*8.);
                
                float2 octahedralUVOfNormalTexture_Int = float2(radianceProbeBaseUV) + octahedralUVOfNormal * float2((radianceProbeResolution-2) * 0.5f, (radianceProbeResolution-2) * 0.5f);
                float2 octahedralUVOfNormalTexture_Float = float2(octahedralUVOfNormalTexture_Int) * radianceProbeResolutionInv;
                //octahedralUVOfNormalTexture_Float = float2(radianceProbeBaseUV) * radianceProbeResolutionInv;
                //float2 octahedralUVOfNormalTexture_Float = float2(octahedralUVOfNormalTexture_Int) * radianceProbeResolutionInv;
                
                //float2 octahedralUVOfNormalTexture_Float = float2(radianceProbeBaseUV) * radianceProbeResolutionInv;
                //octahedralUVOfNormalTexture_Float += 0.5f * float2((depthProbeResolution-2.f), (depthProbeResolution-2.f)) * radianceProbeResolutionInv;
                
                //float3 radiance = VolumeProbeDatasRadiance.Sample(linearSampler, octahedralUVOfNormalTexture_Float).rgb;
                float3 radiance = VolumeProbeDatasRadiance.Sample(linearSampler, octahedralUVOfNormalTexture_Float).rgb;
                //radiance = radiance * (1e-16) + float3(1.f, 1.f, 1.f);
                float2 depths = 2.f * VolumeProbeDatasDepth.Sample(linearSampler, octahedralUVOfDirectioninTexture_Float).rg;
                
                output.radianceProbeUV = float3(octahedralUVOfNormalTexture_Float, 0.f);
                output.depthProbeUV = float3(radianceProbeBaseUV, 0.f);
                output.probeRadiance = radiance;

                float  depth = depths.x;
                float  depthSquared = depths.y;

                float distance = length(probePosition - biasedPos);
                
                float trilinearWeight = TrilinearInterpolationWeight(
                    biasedPos - probeBasePosition, int3(x, y, z), probeSpacing);
                float directionWeight = DirectionWeight(fragNormal, fragPos, probePosition);
                float chebyshevWeight = ChebyshevTest(depth, depthSquared, distance);

                float combinedWeight = directionWeight * chebyshevWeight;
                combinedWeight = max(1e-10, combinedWeight);

                //combinedWeight = 1.f + (1e-16) * combinedWeight;

                combinedWeight = trilinearWeight * combinedWeight;

                //combinedWeight = 1.f * (probeOffsetinVolumn - probeOffsetBase) + (1e-16) * combinedWeight;

                //combinedWeight = 1.f * chebyshevWeight + (1e-16) * combinedWeight;
                

                const float crushThreshold = 0.2f;
                if (combinedWeight < crushThreshold)
                {
                    combinedWeight *= (combinedWeight * combinedWeight) * (1.f / (crushThreshold * crushThreshold));
                }
                totalRadiance += radiance * combinedWeight;
                totalWeights += combinedWeight;
                //break;
            }
            //break;
        }   
        //break;
    }
    //totalRadiance = (1e-10) * totalRadiance + totalWeights;
    //return totalRadiance;

    if(totalWeights == 0.f) return output;

    totalRadiance /= max(totalWeights, 1e-10);
    totalRadiance *= (2.f * PI);

    output.radiance = totalRadiance;

    //totalRadiance = min(max(totalRadiance * (1e-30), 0.01f), 0.f) + test;

    return output;
}