#include "Common.h"
#include "indirectLight.hlsli"

[[vk::binding(1, 0)]]
cbuffer ddgiBuffer : register(b1)
{
    DDGIBuffer ddgiBuffer;
};

[[vk::binding(3, 0)]] StructuredBuffer<DDGIProbe> probes : register(t0);
[[vk::binding(4, 0)]] Texture2D<float4> VolumeProbeDatasRadiance  : register(t0); 
[[vk::binding(5, 0)]] SamplerState linearSampler : register(s0);

struct PSInput 
{
    float3 normal : NORMAL;
    float3 worldPos : POSITION; 
    float2 texCoord : TEXCOORD0;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;  
};

struct PSOutput
{
    [[vk::location(0)]] float4 color : SV_TARGET0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    if(probes[input.instanceID].probeState == PROBE_STATE_INACTIVE){
        output.color = float4(1.f, 0.f, 0.f, 0.5f);
        return output;
    }   

    float2 radianceProbeResolutionInv = 1.f / float2(RadianceProbeResolution * ddgiBuffer.probeDim.x * ddgiBuffer.probeDim.y, RadianceProbeResolution * ddgiBuffer.probeDim.z);

    int probeIndex = input.instanceID;
    //int2 probeBaseIndex = int2(probeIndex / 8, probeIndex % 8);
    int2 probeBaseIndex = int2(probeIndex / ddgiBuffer.probeDim.z, probeIndex % ddgiBuffer.probeDim.z);

    float2 octahedralUVOfNormal = DDGIGetOctahedralCoordinates(input.normal);
    float2 baseUV = probeBaseIndex * float2(RadianceProbeResolution, RadianceProbeResolution) + float2(RadianceProbeResolution*0.5f, RadianceProbeResolution*0.5f);
    float2 octahedralUVOfNormalTexture_Int = float2(baseUV) + octahedralUVOfNormal * 0.5f * float2((RadianceProbeResolution-2.f), (RadianceProbeResolution-2.f));
    float2 octahedralUVOfNormalTexture_Float = float2(octahedralUVOfNormalTexture_Int) * radianceProbeResolutionInv;
                  
    float3 probeRadiance = VolumeProbeDatasRadiance.Sample(linearSampler, octahedralUVOfNormalTexture_Float).rgb;
    //probeRadiance = (1e-20) * probeRadiance + float3((octahedralUV + float2(1.f, 1.f)) / 2.f, 0.f);
    
    output.color = float4(probeRadiance, 0.5f);
    //output.color = float4(input.normal + 0.000000001f*probeRadiance, 0.5);
    return output;
}