#include "SDF.hlsli"

struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET0;
};

struct UniformBuffer0
{
    float3 aabbMin;
    float padding;
    float3 aabbMax;
    float padding2;

    uint3 gridResolution;
	float padding3;
};

struct UniformBuffer1{
    float3 cameraPos;
    float padding;

    float3 cameraDir;
    uint maxRaymarchSteps;

    int2 ScreenResolution;
    float fovY;
    float zNear;
};

struct UniformBuffer2{
    float3 lightDir;
    float padding2;

    float3 lightColor;
    float lightIntensity;
};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    UniformBuffer0 ubo0;
};

[[vk::binding(1, 0)]]
cbuffer ubo1 : register(b1)
{
    UniformBuffer1 ubo1;
};

[[vk::binding(2, 0)]] RWTexture3D<float> SDFTexture : register(u0);
[[vk::binding(3, 0)]] RWTexture3D<float4> albedoTexture : register(u1);

float3 GetPixelDirection(float2 uv){
    float3 UP = float3(0.f, 1.f, 0.f);
    float aspectRatio = (float)ubo1.ScreenResolution.x / (float)ubo1.ScreenResolution.y;

    float3 forward = normalize(ubo1.cameraDir);
    float3 right = normalize(cross(forward, UP));
    float3 up = normalize(cross(right, forward));

    float halfHeight = tan(ubo1.fovY * 0.5f) * ubo1.zNear;
    float halfWidth = aspectRatio * halfHeight;

    float3 zPlaneCenter = ubo1.zNear * forward + ubo1.cameraPos;
    float3 zPlaneCorner = zPlaneCenter - right * halfWidth + up * halfHeight;

    //float2 pixelOffset = 0.5f / float2(ubo1.ScreenResolution);
    float3 pixelPos = zPlaneCorner + right * uv.x * 2 * halfWidth - up * uv.y * 2 * halfHeight;

    return normalize(pixelPos - ubo1.cameraPos);
}

PSOutput main(PSInput input)
{
    PSOutput output;
    //RayMarchSDFStruct struc;

    //struc.step = 0;
    float3 camDir = GetPixelDirection(input.texCoord);
    //float nearestDis = GetNearestDistance(ubo1.cameraPos, camDir, ubo0.aabbMin, ubo0.aabbMax, struc.depth);
    //depth = 0.f;

    float depth = 0.f;

    MDFStruct mdf;
    mdf.SDFTexture = SDFTexture;
    mdf.albedoTexture = albedoTexture;
    mdf.aabbMin = ubo0.aabbMin;
    mdf.aabbMax = ubo0.aabbMax;
    mdf.gridResolution = ubo0.gridResolution;
    
    uint3 res = RayMarchSDF(ubo1.cameraPos, camDir, mdf, ubo1.maxRaymarchSteps, depth);
    
    output.Color = float4(0.f, 0.f, 0.f, 1.f);

    if( (res.x == 1 && res.y == 1 && res.z == 1)){
        float3 hitPoint = ubo1.cameraPos + camDir * depth;
        float3 posCoord = (hitPoint - ubo0.aabbMin) / (ubo0.aabbMax - ubo0.aabbMin);
        int3 coord = posCoord * ubo0.gridResolution;
        float3 albedo = albedoTexture[coord].rgb;

        output.Color = float4(albedo, 1.f);
    }

    return output;
}