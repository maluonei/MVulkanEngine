#include "Octahedral.hlsli"

struct Probe{
    float3 position;
    int padding0;
};


struct UniformBuffer1
{
    //Probe probes[2040];
    int3  probeDim;
    int   raysPerProbe;

    float3 probePos0;
	int	   padding0;
	float3 probePos1;
	int	   padding1;
};

int GetProbeIndex(int3 pIndex, int3 probeScale){
    return pIndex.x * probeScale.y * probeScale.z + pIndex.y * probeScale.z + pIndex.z;
}

float TrilinearInterpolationWeight(float3 gridSpaceDistance, int3 probeIndex, float3 probeSpacing){
    float3 alpha = clamp((gridSpaceDistance / probeSpacing), float3(0.f, 0.f, 0.f), float3(1.f, 1.f, 1.f));

    float3 trilinear = max(0.001f, lerp(1.f - alpha, alpha, probeIndex));
    float  trilinearWeight = (trilinear.x * trilinear.y * trilinear.z);

    return trilinearWeight;
}

float DirectionWeight(float3 normal, float3 position, float3 probePosition){
    float3 direction = normalize(probePosition - position);
    float cosTheta = dot(direction, normal);

    float weight = (cosTheta+1.f) * 0.5f;
    return 0.2f + weight * weight;

}

float ChebyshevTest(float depth, float depthSquared, float distance){
    if(distance < depth){
        return 1.f;
    }
    
    float variance = depthSquared - (depth * depth);

    float v = distance - depth;

    float p = variance / (variance + v * v);

    return max(p * p * p, 0.05f);
}


#define RadianceProbeResolution 8
#define DepthProbeResolution 16
#define surfaceBias 0.02f

struct IndirectLightingOutput{
    float3 radiance;
    float3 radianceProbeUV;
    float3 depthProbeUV;
    float3 probeRadiance;
};

IndirectLightingOutput CalculateIndirectLighting(
    UniformBuffer1 ubo1,
    StructuredBuffer<Probe> probes,
    Texture2D<float4> VolumeProbeDatasRadiance,
    Texture2D<float4> VolumeProbeDatasDepth,
    SamplerState linearSampler,
    float3 probePos0,
    float3 probePos1,
    float3 fragPos, 
    float3 fragNormal){
    
    float3 biasedPos = fragPos + fragNormal * surfaceBias;
    float3 probeSpacing = (probePos1 - probePos0) / (float3(ubo1.probeDim.x, ubo1.probeDim.y, ubo1.probeDim.z) - float3(1.f, 1.f, 1.f));
    //int3   baseProbeCoords = DDGIGetBaseProbeGridCoords(biasedWorldPosition, volume);

    float3 probeOffset = saturate((biasedPos - probePos0) / (probePos1 - probePos0));
    float3 probeOffsetinVolumn = float3(probeOffset * 7);
    int3   probeOffsetBase = int3(probeOffsetinVolumn);
    int3   baseProbeCoords = int3(probeOffsetBase);

    float2 radianceProbeResolutionInv = 1.f / float2(RadianceProbeResolution * ubo1.probeDim.x * ubo1.probeDim.y, RadianceProbeResolution * ubo1.probeDim.z);
    float2 depthProbeResolutionInv = 1.f / float2(DepthProbeResolution * ubo1.probeDim.x * ubo1.probeDim.y, DepthProbeResolution * ubo1.probeDim.z);
    //int probeIndex = GetProbeIndex(probeOffsetInt);

    float totalWeights = 0.f;
    float3 totalRadiance = float3(0.f, 0.f, 0.f);
    float3 test = float3(0.f, 0.f, 0.f);

    float2 octahedralUVOfNormal = DDGIGetOctahedralCoordinates(fragNormal);

    int pBaseIndex = GetProbeIndex(probeOffsetBase, ubo1.probeDim);
    float3 probeBasePosition = probes[pBaseIndex].position;
    
    IndirectLightingOutput output;

    for(int x=0;x<2;x++){
        for(int y=0;y<2;y++){
            for(int z=0;z<2;z++){
    //for(int x=0;x<1;x++){
    //    for(int y=0;y<1;y++){
    //        for(int z=0;z<1;z++){
                //int3 probeBaseIndex = probeOffsetBase + int3(x, y, z);
                int3 probeBaseIndex = clamp(baseProbeCoords + int3(x, y, z), int3(0, 0, 0), int3(ubo1.probeDim.x, ubo1.probeDim.y, ubo1.probeDim.z) - int3(1, 1, 1));

                int pIndex = GetProbeIndex(probeBaseIndex, ubo1.probeDim);

                float3 probePosition = probes[pIndex].position;
                float3 direction = normalize(biasedPos - probePosition);
                
                uint2  radianceProbeBaseUV = uint2(RadianceProbeResolution, RadianceProbeResolution) * uint2(probeBaseIndex.x * ubo1.probeDim.z + probeBaseIndex.y, probeBaseIndex.z) + 0.5f * uint2(RadianceProbeResolution, RadianceProbeResolution);
                uint2  depthProbeBaseUV = uint2(DepthProbeResolution, DepthProbeResolution) * uint2(probeBaseIndex.x * ubo1.probeDim.z + probeBaseIndex.y, probeBaseIndex.z) + 0.5f * uint2(DepthProbeResolution, DepthProbeResolution);

                float2 octahedralUVOfDirection = DDGIGetOctahedralCoordinates(direction);
                //octahedralUVOfDirection = (octahedralUVOfDirection + 1.f) / 2.f;
                float2 octahedralUVOfDirectioninTexture_Int = float2(depthProbeBaseUV) + octahedralUVOfDirection * 0.5f * float2((DepthProbeResolution-2.f), (DepthProbeResolution-2.f));
                float2 octahedralUVOfDirectioninTexture_Float = float2(octahedralUVOfDirectioninTexture_Int) * depthProbeResolutionInv;
                
                float2 octahedralUVOfNormalTexture_Int = float2(radianceProbeBaseUV) + octahedralUVOfNormal * 0.5f * float2((RadianceProbeResolution-2.f), (RadianceProbeResolution-2.f));
                float2 octahedralUVOfNormalTexture_Float = float2(octahedralUVOfNormalTexture_Int) * radianceProbeResolutionInv;
                
                //float3 radiance = VolumeProbeDatasRadiance.Sample(linearSampler, octahedralUVOfNormalTexture_Float).rgb;
                float3 radiance = VolumeProbeDatasRadiance.Sample(linearSampler, octahedralUVOfNormalTexture_Float).rgb;
                float2 depths = VolumeProbeDatasDepth.Sample(linearSampler, octahedralUVOfDirectioninTexture_Float).rg;
                
                output.radianceProbeUV = float3(octahedralUVOfNormalTexture_Float, 0.f);
                output.depthProbeUV = float3(float2(octahedralUVOfNormal * 0.5f * float2((RadianceProbeResolution-2.f), (RadianceProbeResolution-2.f))), 0.f);
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
                //combinedWeight = 1.f + (1e-20) * combinedWeight;

                combinedWeight = trilinearWeight * combinedWeight;

                //const float crushThreshold = 0.2f;
                //if (combinedWeight < crushThreshold)
                //{
                //    combinedWeight *= (combinedWeight * combinedWeight) * (1.f / (crushThreshold * crushThreshold));
                //}
                output.depthProbeUV = float3(directionWeight, chebyshevWeight, combinedWeight);

                totalRadiance += radiance * combinedWeight;
                totalWeights += combinedWeight;
                //break;
            }
            //break;
        }   
        //break;
    }

    if(totalWeights == 0.f) return output;

    totalRadiance /= max(totalWeights, 1e-10);
    totalRadiance *= (2.f * PI);

    output.radiance = totalRadiance;

    //totalRadiance = min(max(totalRadiance * (1e-30), 0.01f), 0.f) + test;

    return output;
}