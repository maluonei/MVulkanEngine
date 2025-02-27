#include "Octahedral.hlsl"

//ProbeData:irradiance, depth, depthSquared

struct Probe{
    float3 position;
    int padding0;
};

struct UniformBuffer0
{
    Probe probes[512];
};

[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UniformBuffer0 ubo0;
};

[[vk::binding(1, 0)]]Texture2D<float4> VolumeProbePosition : register(t0);  //[144, 512]
[[vk::binding(2, 0)]]Texture2D<float4> VolumeProbeRadiance : register(t1);  //[144, 512]
[[vk::binding(3, 0)]]RWTexture2D<float4> VolumeProbeDatasRadiance  : register(u0);   //[512, 64]
[[vk::binding(4, 0)]]RWTexture2D<float4> VolumeProbeDatasDepth  : register(u1);   //[2048, 256]
[[vk::binding(5, 0)]]SamplerState linearSampler : register(s0);


uint2 CalculateProbeBaseIndex(uint2 DispatchThreadID, uint2 probeResolution){
    return DispatchThreadID.xy / probeResolution;
}

uint CalculateProbeIndex(uint2 DispatchThreadID, uint2 probeResolution){
    uint2 baseIndex = CalculateProbeBaseIndex(DispatchThreadID, probeResolution);
    return baseIndex.x * 8 + baseIndex.y;
}

uint2 CalculateProbeOffset(uint2 DispatchThreadID, uint probeResolution){
    return DispatchThreadID.xy % uint2(probeResolution, probeResolution);
}

#define sharpness 50
#define radianceProbeResolution 8
#define depthProbeResolution 16

bool CalculateRadiance(
    uint2 dispatchThreadID, 
    uint probeResolution){

    int probeIndex = CalculateProbeIndex(dispatchThreadID, probeResolution);
    uint2 probeOffset = CalculateProbeOffset(dispatchThreadID, probeResolution);

    float3 probePosition = ubo0.probes[probeIndex].position;
    float2 octahedralUV = probeOffset.xy / float2(probeResolution, probeResolution);
    float weights = 0.f;

    if(probeOffset.x!=0 && probeOffset.x!=probeResolution && probeOffset.y!=0 && probeOffset.y!=probeResolution){
        float3 sphereDirection = DDGIGetOctahedralDirection(octahedralUV);

        float3 irradiance = float3(0.f, 0.f, 0.f);
        float3 totalWeights = 0.f;
        for(int i=0;i<64;i++){
            float2 volumeProbePositionUV = float2(i, probeIndex) / float2(64., 512.);
            float3 targetPosition = VolumeProbePosition.Sample(linearSampler, volumeProbePositionUV).rgb;
            float3 targetRadiance = VolumeProbeRadiance.Sample(linearSampler, volumeProbePositionUV).rgb;

            float cosTheta = abs(dot(sphereDirection, normalize(targetPosition - probePosition)));

            float weight = pow(cosTheta, sharpness);
            irradiance += targetRadiance * weight;
            weights += weight;
        }

        irradiance = irradiance / weights;
        VolumeProbeDatasRadiance[dispatchThreadID.xy] = float4(irradiance, 1.f);
        //return;

        return true;
    }

    return false;
}

bool CalculateDepth(
    uint2 dispatchThreadID, 
    uint probeResolution){

    int probeIndex = CalculateProbeIndex(dispatchThreadID, probeResolution);
    uint2 probeOffset = CalculateProbeOffset(dispatchThreadID, probeResolution);

    float3 probePosition = ubo0.probes[probeIndex].position;
    float2 octahedralUV = probeOffset.xy / float2(probeResolution, probeResolution);
    float weights = 0.f;

    if(probeOffset.x!=0 && probeOffset.x!=probeResolution && probeOffset.y!=0 && probeOffset.y!=probeResolution){
        //float2 octahedralUV = probeOffset.xy / float2(8.f, 8.f);
        
        float3 sphereDirection = DDGIGetOctahedralDirection(octahedralUV);

        float depth = 0.f;
        float depthSquared = 0.f;

        float3 totalWeights = 0.f;
        for(int i=0;i<64;i++){
            float2 volumeProbePositionUV = float2(i, probeIndex) / float2(64., 512.);

            float4 targetPositions = VolumeProbePosition.Sample(linearSampler, volumeProbePositionUV).rgba;
            
            float3 targetPosition = targetPositions.rgb;
            float targetDepth = targetPositions.a;
            float targetDepthSquared = targetDepth * targetDepth;

            float cosTheta = abs(dot(sphereDirection, normalize(targetPosition - probePosition)));

            float weight = pow(cosTheta, sharpness);
            depth += targetDepth * weight;
            depthSquared += targetDepthSquared * weight;
            weights += weight;
        }

        depth = depth / weights;
        depthSquared = depthSquared / weights;
        VolumeProbeDatasDepth[dispatchThreadID.xy] = float4(depth, depthSquared, 0.f, 0.f);
        return true;
    }

    return false;
}

void UpdateRadianceBorderPixel(
    uint2 dispatchThreadID, 
    uint probeResolution){
    
    uint2 probeOffset = CalculateProbeOffset(dispatchThreadID, probeResolution);

    bool isCornerPixel = (probeOffset.x==0 || probeOffset.x==probeResolution-1) && (probeOffset.y==0 || probeOffset.y==probeResolution-1);
    bool isRowTexel = probeOffset.x > 0 && probeOffset.x < probeResolution;
    //bool isColumnTexel = probeOffset.y > 0 && probeOffset.y < probeResolution;

    uint2 probeBaseIndex = dispatchThreadID - probeOffset;
    uint2 copyCoordinates = probeBaseIndex;
    //uint2 probeBaseIndexNext = probeBaseIndex + uint2(probeResolution, probeResolution);

    if(isCornerPixel){
        copyCoordinates.x += probeOffset.x == 0 ? probeResolution - 2 : 1;
        copyCoordinates.y += probeOffset.y == 0 ? probeResolution - 2 : 1;
    }
    else if(isRowTexel){
        copyCoordinates.x += (probeResolution - 1 - probeOffset.x);
        copyCoordinates.y += probeOffset.y + ((probeOffset.y > 1) ? -1 : 1);
    }
    else{ //columnPixel
        copyCoordinates.y += (probeResolution - 1 - probeOffset.y);
        copyCoordinates.x += probeOffset.x + ((probeOffset.x > 1) ? -1 : 1);
    }

    VolumeProbeDatasRadiance[dispatchThreadID.xy] = VolumeProbeDatasRadiance.Load(uint3(copyCoordinates, 0)).rgba;
}

void UpdateDepthBorderPixel(
    uint2 dispatchThreadID, 
    uint probeResolution){
    
    uint2 probeOffset = CalculateProbeOffset(dispatchThreadID, probeResolution);

    bool isCornerPixel = (probeOffset.x==0 || probeOffset.x==probeResolution-1) && (probeOffset.y==0 || probeOffset.y==probeResolution-1);
    bool isRowTexel = probeOffset.x > 0 && probeOffset.x < probeResolution;
    //bool isColumnTexel = probeOffset.y > 0 && probeOffset.y < probeResolution;

    uint2 probeBaseIndex = dispatchThreadID - probeOffset;
    uint2 copyCoordinates = probeBaseIndex;
    //uint2 probeBaseIndexNext = probeBaseIndex + uint2(probeResolution, probeResolution);

    if(isCornerPixel){
        copyCoordinates.x += probeOffset.x == 0 ? probeResolution - 2 : 1;
        copyCoordinates.y += probeOffset.y == 0 ? probeResolution - 2 : 1;
    }
    else if(isRowTexel){
        copyCoordinates.x += (probeResolution - 1 - probeOffset.x);
        copyCoordinates.y += probeOffset.y + ((probeOffset.y > 1) ? -1 : 1);
    }
    else{ //columnPixel
        copyCoordinates.y += (probeResolution - 1 - probeOffset.y);
        copyCoordinates.x += probeOffset.x + ((probeOffset.x > 1) ? -1 : 1);
    }

    VolumeProbeDatasDepth[dispatchThreadID.xy] = VolumeProbeDatasDepth.Load(uint3(copyCoordinates, 0)).rgba;
}

//[64, 8]
[numthreads(8, 8, 1)] 
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    //DispatchThreadID:[2048, 256, 1]
    //DispatchThreadID:[512, 64, 1]
    //uint2 radianceProbeResolution = uint2(radianceProbeResolution, radianceProbeResolution);
    //uint2 depthProbeResolution = uint2(depthProbeResolution, depthProbeResolution);

    uint2 radianceDispatchThreadID = DispatchThreadID.xy / uint2(2, 2);
    bool calculateRadiance = DispatchThreadID.x % 2 == 0 && DispatchThreadID.y % 2 == 0;

    bool cd = CalculateDepth(DispatchThreadID.xy, depthProbeResolution);
    bool cr = false;
    if(calculateRadiance){
        cr = CalculateRadiance(radianceDispatchThreadID, radianceProbeResolution);
    }

    GroupMemoryBarrierWithGroupSync();

    if(cr){
        UpdateRadianceBorderPixel(radianceDispatchThreadID, radianceProbeResolution);
        UpdateDepthBorderPixel(DispatchThreadID.xy, depthProbeResolution);
    }
    else if(cd){
        UpdateDepthBorderPixel(DispatchThreadID.xy, depthProbeResolution);
    }
}