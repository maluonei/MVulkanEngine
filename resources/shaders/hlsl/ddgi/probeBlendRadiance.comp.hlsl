//#include "Octahedral.hlsli"
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
[[vk::binding(3, 0)]]Texture2D<float4> VolumeProbePosition : register(t0);  //[raysPerProbe, probeDim.x*probeDim.y*probeDim.z]
[[vk::binding(4, 0)]]Texture2D<float4> VolumeProbeRadiance : register(t1);  //[raysPerProbe, probeDim.x*probeDim.y*probeDim.z]
[[vk::binding(5, 0)]]RWTexture2D<float4> VolumeProbeDatasRadiance  : register(u0);   //[probeDim.x * probeDim.y * 8, probeDim.z * 8]
[[vk::binding(6, 0)]]RWTexture2D<float4> VolumeProbeDatasDepth  : register(u1);      //[probeDim.x * probeDim.y * 16, probeDim.z * 16]
[[vk::binding(7, 0)]]SamplerState linearSampler : register(s0);

#define MAXRAYSPERPROBE 144

groupshared float3 RayRadiance[MAXRAYSPERPROBE];
groupshared float  RayDistance[MAXRAYSPERPROBE];
groupshared float3 RayDirection[MAXRAYSPERPROBE];
//#define ProbeResolution 8
//#define RadianceProbeResolution 8
//#define DepthProbeResolution 16
//#define raysPerProbe 144

uint2 CalculateProbeBaseIndex(uint2 DispatchThreadID, uint2 probeResolution){
    return DispatchThreadID.xy / probeResolution;
}

uint CalculateProbeIndex(uint2 DispatchThreadID, uint2 probeResolution, uint3 probeDim){
    uint2 baseIndex = CalculateProbeBaseIndex(DispatchThreadID, probeResolution);
    return baseIndex.x * probeDim.z + baseIndex.y;
}

uint2 CalculateProbeOffset(uint2 DispatchThreadID, uint2 probeResolution){
    return DispatchThreadID.xy % probeResolution;
}

bool CalculateRadiance(
    uint2 dispatchThreadID, 
    uint probeResolution, uint3 probeDim){

    int probeIndex = CalculateProbeIndex(dispatchThreadID, probeResolution, probeDim);

    uint2 probeOffset = CalculateProbeOffset(dispatchThreadID, probeResolution);

    float3 probePosition = probes[probeIndex].position;
    if(probeOffset.x!=0 && probeOffset.x!=(probeResolution-1) && probeOffset.y!=0 && probeOffset.y!=(probeResolution-1)){

        int2 threadCoords = probeOffset.xy - int2(1, 1);
        float2 octahedralTexelCoord = threadCoords + 0.5f;
        octahedralTexelCoord /= float(probeResolution-2);
        octahedralTexelCoord *= 2.f;
        float2 octahedralUV = octahedralTexelCoord - float2(1.f, 1.f);

        float3 sphereDirection = DDGIGetOctahedralDirection(octahedralUV);

        float3 irradiance = float3(0.f, 0.f, 0.f);

        float totalIradianceWeights = 0.f;
        for(int i=0;i<ubo1.raysPerProbe;i++){
            //float2 volumeProbePositionUV = float2(i, probeIndex) / float2(64.f - 1.f, 512.f - 1.f);
            int3 volumeProbePositionUV_Int = int3(i, probeIndex, 0);
        
            //float4 targetPositions = VolumeProbePosition.Load(volumeProbePositionUV_Int).rgba;
            //float4 targetRadiances = VolumeProbeRadiance.Load(volumeProbePositionUV_Int).rgba;
//
            //float3 targetPosition = targetPositions.rgb;
            //float3 targetRadiance = targetRadiances.rgb;
            //float cosTheta = max(0.f, dot(sphereDirection, normalize(targetPosition - probePosition)));
            float3 radiance = RayRadiance[i];
            //float distance = RayDistance[i];
            float3 direction = RayDirection[i];

            float cosTheta = max(0.f, dot(sphereDirection, direction));
            //if(cosTheta > 0.f){
                float irradianceWeight = cosTheta * ubo0.sharpness;
                //irradiance += targetRadiance * irradianceWeight;
                irradiance += radiance * irradianceWeight;
                totalIradianceWeights += irradianceWeight;
            //}
        }
        
        irradiance = irradiance / max(2.f * totalIradianceWeights, 1e-8);

        VolumeProbeDatasRadiance[dispatchThreadID.xy] = float4(irradiance, 1.f);
        //VolumeProbeDatasRadiance[dispatchThreadID.xy] = float4(irradiance * (1e-20) + float3(probeIndex, 0, 0) / 512.f, 1.f);

        return true;
    }

    return false;
}

bool CalculateDepth( 
    uint2 dispatchThreadID, 
    uint probeResolution, uint3 probeDim){

    int probeIndex = CalculateProbeIndex(dispatchThreadID, probeResolution, probeDim);
    uint2 probeOffset = CalculateProbeOffset(dispatchThreadID, probeResolution);

    float3 probePosition = probes[probeIndex].position;
    //float2 octahedralUV = probeOffset.xy / float2(probeResolution, probeResolution);
    //float weights = 0.f;

    if(probeOffset.x!=0 && probeOffset.x!=(probeResolution-1) && probeOffset.y!=0 && probeOffset.y!=(probeResolution-1)){

        int2 threadCoords = probeOffset.xy - int2(1, 1);
        float2 octahedralTexelCoord = threadCoords + 0.5f;
        octahedralTexelCoord /= float(probeResolution-2);
        octahedralTexelCoord *= 2.f;
        float2 octahedralUV = octahedralTexelCoord - float2(1.f, 1.f);

        float3 sphereDirection = DDGIGetOctahedralDirection(octahedralUV);

        //float3 irradiance = float3(0.f, 0.f, 0.f);
        float depth = 0.f;
        float depthSquared = 0.f;

        //float totalIradianceWeights = 0.f;
        float totalDepthWeights = 0.f;

        for(int i=0;i<ubo1.raysPerProbe;i++){
            int3 volumeProbePositionUV_Int = int3(i, probeIndex, 0);
        
            //float4 targetPositions = VolumeProbePosition.Load(volumeProbePositionUV_Int).rgba;
            //float4 targetRadiances = VolumeProbeRadiance.Load(volumeProbePositionUV_Int).rgba;;
//
            //float3 targetPosition = targetPositions.rgb;
            //float targetDepth = targetRadiances.a;
            //float targetDepthSquared = targetDepth * targetDepth;
            //float cosTheta = max(0.f, dot(sphereDirection, normalize(targetPosition - probePosition)));

            //float3 radiance = RayRadiance[i];
            float distance = RayDistance[i];
            float3 direction = RayDirection[i];

            float targetDepth = distance;
            float targetDepthSquared = distance * distance;
            float cosTheta = max(0.f, dot(sphereDirection, direction));

            //if(cosTheta > 0.f){
                float depthWeight = pow(cosTheta, ubo0.sharpness);
                depth += targetDepth * depthWeight;
                depthSquared += targetDepthSquared * depthWeight;
                totalDepthWeights += depthWeight;
            //}
        }
        
        depth = depth / max(totalDepthWeights, 1e-10);
        depthSquared = depthSquared / max(totalDepthWeights, 1e-10);

        VolumeProbeDatasDepth[dispatchThreadID.xy] = float4(depth, depthSquared, 0.f, 0.f);

        return true;
    }

    return false;
}

void UpdateBorderPixelDepth(
    uint2 dispatchThreadID, 
    uint probeResolution){
    
    uint2 probeOffset = CalculateProbeOffset(dispatchThreadID, probeResolution);

    bool isCornerPixel = (probeOffset.x==0 || probeOffset.x==probeResolution-1) && (probeOffset.y==0 || probeOffset.y==probeResolution-1);
    bool isRowTexel = probeOffset.x > 0 && probeOffset.x < (probeResolution-1);

    uint2 probeBaseIndex = dispatchThreadID - probeOffset;
    uint2 copyCoordinates = probeBaseIndex;

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

void UpdateBorderPixelRadiance(
    uint2 dispatchThreadID, 
    uint probeResolution){
    
    uint2 probeOffset = CalculateProbeOffset(dispatchThreadID, probeResolution);

    bool isCornerPixel = (probeOffset.x==0 || probeOffset.x==probeResolution-1) && (probeOffset.y==0 || probeOffset.y==probeResolution-1);
    bool isRowTexel = probeOffset.x > 0 && probeOffset.x < (probeResolution-1);

    uint2 probeBaseIndex = dispatchThreadID - probeOffset;
    uint2 copyCoordinates = probeBaseIndex;

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

void LoadSharedMemory(int probeIndex, uint GroupIndex, int pixelsPerProbe){
    int totalIterations = int(ceil(float(ubo1.raysPerProbe) / float(pixelsPerProbe)));
    for(int i=0;i<totalIterations;i++){
        int rayIndex = (GroupIndex * totalIterations) + i;
        if (rayIndex >= ubo1.raysPerProbe) break;

        int3 volumeProbePositionUV_Int = int3(rayIndex, probeIndex, 0);
        float4 targetPositions = VolumeProbePosition.Load(volumeProbePositionUV_Int).rgba;
        float4 targetRadiances = VolumeProbeRadiance.Load(volumeProbePositionUV_Int).rgba;

        RayRadiance[rayIndex] = targetRadiances.rgb;
        RayDistance[rayIndex] = targetRadiances.a;
        RayDirection[rayIndex] = normalize(targetPositions.rgb - probes[probeIndex].position);
    }
    //GroupMemoryBarrierWithGroupSync();
}

//[64, 8]
[numthreads(8, 8, 1)] 
void main_radiance(  uint3 DispatchThreadID : SV_DispatchThreadID,
            uint  GroupIndex       : SV_GroupIndex)
{
    int probeIndex = CalculateProbeIndex(DispatchThreadID, DepthProbeResolution, ubo1.probeDim);
    //if(probes[probeIndex].probeState == PROBE_STATE_INACTIVE) return;

    LoadSharedMemory(probeIndex, GroupIndex, 8*8);
    GroupMemoryBarrierWithGroupSync();
    
    //DispatchThreadID:[2048, 256, 1]
    //DispatchThreadID:[512, 64, 1]
    uint2 radianceProbeResolution = uint2(RadianceProbeResolution, RadianceProbeResolution);
    //uint2 depthProbeResolution = uint2(DepthProbeResolution, DepthProbeResolution);

    //bool calculateRadiance = false;
    bool radianceIsCorner = false;
    //if(DispatchThreadID.x%2==0 && DispatchThreadID.y%2==0){
        //calculateRadiance = true;
    radianceIsCorner = !(CalculateRadiance(DispatchThreadID.xy/int2(2,2), radianceProbeResolution, ubo1.probeDim));
    //}

    //bool depthIsCorner = !(CalculateDepth(DispatchThreadID.xy, depthProbeResolution, ubo1.probeDim));

    //return;

    GroupMemoryBarrierWithGroupSync();

    //if(depthIsCorner){
    //    UpdateBorderPixelDepth(DispatchThreadID.xy, depthProbeResolution);
    //}
    //if(calculateRadiance){
    if(radianceIsCorner){
        UpdateBorderPixelRadiance(DispatchThreadID.xy/int2(2,2), radianceProbeResolution);
    }
    //}
}

[numthreads(16, 16, 1)] 
void main_depth(  uint3 DispatchThreadID : SV_DispatchThreadID,
            uint  GroupIndex       : SV_GroupIndex)
{
    int probeIndex = CalculateProbeIndex(DispatchThreadID, DepthProbeResolution, ubo1.probeDim);
    //if(probes[probeIndex].probeState == PROBE_STATE_INACTIVE) return;

    LoadSharedMemory(probeIndex, GroupIndex, 16*16);
    GroupMemoryBarrierWithGroupSync();
    
    //DispatchThreadID:[2048, 256, 1]
    //DispatchThreadID:[512, 64, 1]
    //uint2 radianceProbeResolution = uint2(RadianceProbeResolution, RadianceProbeResolution);
    uint2 depthProbeResolution = uint2(DepthProbeResolution, DepthProbeResolution);

    //bool calculateRadiance = false;
    //bool radianceIsCorner = false;
    //if(DispatchThreadID.x%2==0 && DispatchThreadID.y%2==0){
    //    calculateRadiance = true;
    //    radianceIsCorner = !(CalculateRadiance(DispatchThreadID.xy/int2(2,2), radianceProbeResolution, ubo1.probeDim));
    //}

    bool depthIsCorner = !(CalculateDepth(DispatchThreadID.xy, depthProbeResolution, ubo1.probeDim));

    //return;

    GroupMemoryBarrierWithGroupSync();

    if(depthIsCorner){
        UpdateBorderPixelDepth(DispatchThreadID.xy, depthProbeResolution);
    }
    //if(calculateRadiance){
    //    if(radianceIsCorner){
    //        UpdateBorderPixelRadiance(DispatchThreadID.xy/int2(2,2), radianceProbeResolution);
    //    }
    //}
}