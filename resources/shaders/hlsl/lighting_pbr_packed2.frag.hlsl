#include "Common.h"

[[vk::binding(0, 0)]]
cbuffer lightBuffer : register(b0)
{
    LightBuffer lightBuffer;
}

[[vk::binding(1, 0)]]
cbuffer cameraBuffer : register(b1)
{
    MCameraBuffer cameraBuffer;
}

[[vk::binding(2, 0)]]
cbuffer screenBuffer : register(b2)
{
    MScreenBuffer screenBuffer;
}

[[vk::binding(7, 0)]]
cbuffer intBuffer : register(b3)
{
    int outputMotionVector;
}

//#define ShadingShaderConstantBuffer ubo0;

[[vk::binding(3, 0)]]Texture2D<uint4> gBuffer0 : register(t0);
[[vk::binding(4, 0)]]Texture2D<uint4> gBuffer1 : register(t1);
//[[vk::binding(8, 0)]]Texture2D<float4> gBuffer2 : register(t2);
[[vk::binding(5, 0)]]Texture2D<float> shadowMaps[2] : register(t4);
[[vk::binding(6, 0)]]SamplerState linearSampler : register(s0);

struct PSInput
{
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 position : SV_POSITION;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

static const float4x4 biasMat = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
);

static const float DepthBias = 0.001f;
static const float PI = 3.14159265359f;

static const float gaussian_filter[7][7] =
{
    { 0.00002, 0.00024, 0.00107, 0.00177, 0.00107, 0.00024, 0.00002 },
    { 0.00024, 0.00292, 0.01307, 0.02155, 0.01307, 0.00292, 0.00024 },
    { 0.00107, 0.01307, 0.05858, 0.09658, 0.05858, 0.01307, 0.00107 },
    { 0.00177, 0.02155, 0.09658, 0.15924, 0.09658, 0.02155, 0.00177 },
    { 0.00107, 0.01307, 0.05858, 0.09658, 0.05858, 0.01307, 0.00107 },
    { 0.00024, 0.00292, 0.01307, 0.02155, 0.01307, 0.00292, 0.00024 },
    { 0.00002, 0.00024, 0.00107, 0.00177, 0.00107, 0.00024, 0.00002 }
};

static const float2 poisson_points[49] =
{
    float2(0.64695844, 0.51427758),
    float2(-0.46793236, -0.1924265),
    float2(0.83931444, -0.50187218),
    float2(-0.33727795, -0.20351218),
    float2(-0.25757024, 0.62921074),
    float2(0.58200304, 0.6415687),
    float2(-0.1633649, 0.58089455),
    float2(-0.31460482, 0.01513585),
    float2(0.29897802, -0.18115484),
    float2(-0.17050708, -0.29307765),
    float2(0.15745089, 0.68015682),
    float2(-0.33138415, 0.76567117),
    float2(-0.95231302, 0.24739375),
    float2(0.23277111, 0.81925215),
    float2(-0.04367417, 0.30952061),
    float2(-0.4931112, 0.55145933),
    float2(-0.78287756, -0.40832679),
    float2(-0.45266041, 0.26767449),
    float2(-0.19905406, -0.02066478),
    float2(-0.00670141, 0.51514763),
    float2(0.14742235, -0.84160096),
    float2(0.05853309, 0.57925968),
    float2(0.05116486, -0.41123827),
    float2(0.0063878, 0.72016596),
    float2(-0.32452109, -0.89742536),
    float2(0.53531466, -0.32224962),
    float2(-0.05138841, -0.62892213),
    float2(-0.87586551, 0.0030228),
    float2(0.91015664, -0.04491546),
    float2(-0.08270389, -0.74938649),
    float2(-0.60847032, -0.19709423),
    float2(0.69280502, -0.24633194),
    float2(-0.00203186, -0.95271331),
    float2(-0.75505301, -0.45141735),
    float2(0.35192106, -0.87774144),
    float2(-0.47432603, -0.63489716),
    float2(0.5806755, 0.28115667),
    float2(0.6934058, -0.48463034),
    float2(0.30522932, 0.24507918),
    float2(-0.16404957, 0.18318721),
    float2(-0.55109203, -0.4667833),
    float2(-0.38212837, -0.13039155),
    float2(-0.76455652, -0.58506595),
    float2(-0.05517833, -0.58396479),
    float2(-0.29708975, 0.62018128),
    float2(0.52109725, 0.36880081),
    float2(-0.16793336, -0.82001413),
    float2(0.159529, -0.29196939),
    float2(-0.40267215, 0.70380431)
};

float LinearDepth(float depth, float zNear, float zFar)
{
    float zDis = zFar - zNear;
    return (2.f * depth * zDis) / (zFar + zNear - depth * zDis);
}

float PCF(int lightIndex, float3 fragPos)
{
    float occlusion = 0.f;

    float4 shadowCoord = mul(biasMat, mul(lightBuffer.lights[lightIndex].shadowViewProj, float4(fragPos, 1.f)));
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.y = 1.f - shadowCoord.y;

    float shadowDepth = shadowMaps[lightBuffer.lights[lightIndex].shadowMapIndex].Sample(linearSampler, shadowCoord.xy).r;

    for (int i = -3; i <= 3; i++)
    {
        for (int j = -3; j <= 3; j++)
        {
            float2 offset = poisson_points[i * 7 + j + 24] * float2(5.f, 5.f) / float2(lightBuffer.lights[lightIndex].shadowmapResolution);
            float shadowDepthSample = shadowMaps[lightBuffer.lights[lightIndex].shadowMapIndex].Sample(linearSampler, shadowCoord.xy + offset).r;
            if ((shadowDepthSample + DepthBias) < shadowCoord.z)
            {
                occlusion += 1.f;
            }
        }
    }

    occlusion /= 49.f;
    return occlusion;
}

float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function ----------------------------------------------------
float3 F_Schlick(float cosTheta, float metallic)
{
    float3 F0 = lerp(float3(0.04,0.04,0.04), float3(1.f,1.f,1.f), metallic); // * material.specular
    float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    return F;
}

// Specular BRDF composition --------------------------------------------

float3 BRDF(float3 diffuseColor, float3 lightColor, float3 L, float3 V, float3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
    float3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed
	//float3 lightColor = float3(1.0);

    float3 color = float3(0.0, 0.0, 0.0);

    if (dotNL > 0.0)
    {
        float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
        float3 F = F_Schlick(dotNV, metallic);

        float3 spec = D * F * G / ((4.0 * dotNL * dotNV) + 0.0001);
        float3 diff = (1.f - F) * (1.f - metallic) * diffuseColor / PI;

        color += (spec + diff) * dotNL * lightColor;
    }

    return color;
}

float3 rgb2srgb(float3 color)
{
    color.x = color.x < 0.0031308 ? color.x * 12.92f : pow(color.x, 1.0 / 2.4) * 1.055f - 0.055f;
    color.y = color.y < 0.0031308 ? color.y * 12.92f : pow(color.y, 1.0 / 2.4) * 1.055f - 0.055f;
    color.z = color.z < 0.0031308 ? color.z * 12.92f : pow(color.z, 1.0 / 2.4) * 1.055f - 0.055f;

    return color;
}

void Unpack(
    uint4 gBuffer0,
    uint4 gBuffer1,
    out float3 normal,
    out float3 position,
    out float2 uv,
    out float3 albedo,
    out float metallic,
    out float roughness,
    out float3 motionVector
){
    uint3 unpackedNormal = uint3((gBuffer0.x & 0xFFFF0000) >> 16, gBuffer0.x & 0xFFFF, (gBuffer0.y & 0xFFFF0000) >> 16);
    uint3 unpackedPosition = uint3(gBuffer0.y & 0xFFFF, (gBuffer0.z & 0xFFFF0000) >> 16, gBuffer0.z & 0xFFFF);
    uint2 unpackedUV = uint2((gBuffer0.w & 0xFFFF0000) >> 16, gBuffer0.w & 0xFFFF);

    uint3 unpackedAlbedo = uint3((gBuffer1.x & 0xFFFF0000) >> 16, gBuffer1.x & 0xFFFF, (gBuffer1.y & 0xFFFF0000) >> 16);
    uint unpackedMetallic = gBuffer1.y & 0xFFFF;
    uint unpackedRoughness = (gBuffer1.z & 0xFFFF0000) >> 16;
    uint3 unpackedMotionVector = uint3(gBuffer1.z & 0xFFFF, (gBuffer1.w & 0xFFFF0000) >> 16, gBuffer1.w & 0xFFFF);

    normal = normalize(f16tof32(unpackedNormal));
    position = f16tof32(unpackedPosition);
    uv = f16tof32(unpackedUV);
    albedo = f16tof32(unpackedAlbedo);
    metallic = f16tof32(unpackedMetallic);
    roughness = f16tof32(unpackedRoughness);
    motionVector = f16tof32(unpackedMotionVector);
}


PSOutput main(PSInput input)
{
    PSOutput output;

    uint3 coord = uint3(uint2(input.texCoord * float2(screenBuffer.WindowRes.x, screenBuffer.WindowRes.y)), 0);
    uint4 gBufferValue0 = gBuffer0.Load(coord);
    uint4 gBufferValue1 = gBuffer1.Load(coord);
    //float4 gBufferValue2 = gBuffer2.Load(coord);
    float3 fragNormal;
    float3 fragPos;
    float2 fragUV;
    float3 fragAlbedo;
    float metallic;
    float roughness;
    float3 motionVector;

    Unpack(
        gBufferValue0, 
        gBufferValue1, 
        fragNormal, 
        fragPos, 
        fragUV, 
        fragAlbedo, 
        metallic, 
        roughness,
        motionVector);

    float3 fragcolor = float3(0.f, 0.f, 0.f);

    for (int i = 0; i < lightBuffer.lightNum; i++)
    {
        float ocllusion = PCF(i, fragPos);

        float3 L = normalize(-lightBuffer.lights[i].direction);
        float3 V = normalize(cameraBuffer.cameraPos.xyz - fragPos);
        float3 R = reflect(-V, fragNormal);

        fragcolor += (1.f - ocllusion) * BRDF(fragAlbedo.rgb, lightBuffer.lights[i].intensity * lightBuffer.lights[i].color, L, V, fragNormal, metallic, roughness);
    }
    fragcolor += fragAlbedo.rgb * 0.04f; //ambient

    if(outputMotionVector==1){
        output.color = float4(motionVector.xy, 0.f, 1.f);// * 100.f;
    }
    //else if(outputMotionVector==2){
    //    output.color = float4(gBufferValue2.xy, 0.f, 1.f);
    //}
    else{
        output.color = float4(fragcolor, 1.f);
    }

    return output;
}
