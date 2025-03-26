struct Light
{
    float4x4 shadowViewProj;

    float3 direction;
    float intensity;

    float3 color;
    int shadowMapIndex;

    float cameraZnear;
    float cameraZfar;
    float padding6;
    float padding7;
};

struct UnifomBuffer0
{
    Light lights[2];

    float3 cameraPos;
    int lightNum;
    
    int ResolutionWidth;
    int ResolutionHeight;
    int padding0;
    int padding1;
};


[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    UnifomBuffer0 ubo0;
}

[[vk::binding(1, 0)]]Texture2D<float4> gBufferNormal : register(t0);
[[vk::binding(2, 0)]]Texture2D<float4> gBufferPosition : register(t1);
[[vk::binding(3, 0)]]Texture2D<float4> gAlbedo : register(t2);
[[vk::binding(4, 0)]]Texture2D<float4> gMetallicAndRoughness : register(t3);
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

    float4 shadowCoord = mul(biasMat, mul(ubo0.lights[lightIndex].shadowViewProj, float4(fragPos, 1.f)));
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.y = 1.f - shadowCoord.y;

    float shadowDepth = shadowMaps[ubo0.lights[lightIndex].shadowMapIndex].Sample(linearSampler, shadowCoord.xy).r;

    for (int i = -3; i <= 3; i++)
    {
        for (int j = -3; j <= 3; j++)
        {
            float2 offset = poisson_points[i * 7 + j + 24] * float2(5.f, 5.f) / float2(ubo0.ResolutionWidth, ubo0.ResolutionHeight);
            float shadowDepthSample = shadowMaps[ubo0.lights[lightIndex].shadowMapIndex].Sample(linearSampler, shadowCoord.xy + offset).r;
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

PSOutput main(PSInput input)
{
    PSOutput output;
    
    float4 gBufferValue0 = gBufferNormal.Sample(linearSampler, input.texCoord);
    float4 gBufferValue1 = gBufferPosition.Sample(linearSampler, input.texCoord);
    float4 gBufferValue2 = gAlbedo.Sample(linearSampler, input.texCoord);
    float4 gBufferValue3 = gMetallicAndRoughness.Sample(linearSampler, input.texCoord);

    float3 fragNormal = normalize(gBufferValue0.rgb);
    float3 fragPos = gBufferValue1.rgb;
    float2 fragUV = float2(gBufferValue0.a, gBufferValue1.a);
    float4 fragAlbedo = gBufferValue2.rgba;
    float metallic = gBufferValue3.b;
    float roughness = gBufferValue3.g;
    
    float3 fragcolor = float3(0.f, 0.f, 0.f);

    for (int i = 0; i < ubo0.lightNum; i++)
    {
        float ocllusion = PCF(i, fragPos);

        float3 L = normalize(-ubo0.lights[i].direction);
        float3 V = normalize(ubo0.cameraPos.xyz - fragPos);
        float3 R = reflect(-V, fragNormal);

        fragcolor += (1.f - ocllusion) * BRDF(fragAlbedo.rgb, ubo0.lights[i].intensity * ubo0.lights[i].color, L, V, fragNormal, metallic, roughness);
        fragcolor += fragAlbedo.rgb * 0.04f; //ambient
    }

    output.color = float4(fragcolor, 1.f);
    
    return output;
}
