#version 450 core
#extension GL_NV_uniform_buffer_std430_layout : enable
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0)out vec4 color;

layout(binding = 2) uniform sampler2D gBufferNormal;
layout(binding = 3) uniform sampler2D gBufferPosition;
layout(binding = 4) uniform sampler2D gAlbedo;
layout(binding = 5) uniform sampler2D gMetallicAndRoughness;
layout(binding = 6) uniform usampler2D gMatId;
layout(binding = 7) uniform sampler2D shadowMaps[2];

layout(binding = 8) uniform samplerCube enviromentIrradiance;
layout(binding = 9) uniform samplerCube prefilteredEnvmap;
layout(binding = 10) uniform sampler2D brdfLUT;

layout(location = 0)in vec2 texCoord;

struct Light {
    mat4 shadowViewProj;

    vec3 direction;
    float intensity;

    vec3 color;
    int shadowMapIndex;

    float cameraZnear;
    float cameraZfar;
    float padding6;
    float padding7;
};

layout(std430, binding = 1) uniform DirectionalLightBuffer{
    Light lights[2];
    vec3 cameraPos;
    int lightNum;
} ubo1;


layout(std430, binding = 0) uniform UniformBuffer{
    int ResolusionWidth;
    int ResolusionHeight;
    int padding0;
    int padding1;
} ubo0;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0);

const float DepthBias = 0.001f;
const float PI = 3.14159265359;

const float gaussian_filter[7][7] = {
    {0.00002, 0.00024, 0.00107, 0.00177, 0.00107, 0.00024, 0.00002},
    {0.00024, 0.00292, 0.01307, 0.02155, 0.01307, 0.00292, 0.00024},
    {0.00107, 0.01307, 0.05858, 0.09658, 0.05858, 0.01307, 0.00107},
    {0.00177, 0.02155, 0.09658, 0.15924, 0.09658, 0.02155, 0.00177},
    {0.00107, 0.01307, 0.05858, 0.09658, 0.05858, 0.01307, 0.00107},
    {0.00024, 0.00292, 0.01307, 0.02155, 0.01307, 0.00292, 0.00024},
    {0.00002, 0.00024, 0.00107, 0.00177, 0.00107, 0.00024, 0.00002}
};

const vec2 poisson_points[49] = {
    { 0.64695844,  0.51427758},
    {-0.46793236, -0.1924265 },
    { 0.83931444, -0.50187218},
    {-0.33727795, -0.20351218},
    {-0.25757024,  0.62921074},
    { 0.58200304,  0.6415687 },
    {-0.1633649 ,  0.58089455},
    {-0.31460482,  0.01513585},
    { 0.29897802, -0.18115484},
    {-0.17050708, -0.29307765},
    { 0.15745089,  0.68015682},
    {-0.33138415,  0.76567117},
    {-0.95231302,  0.24739375},
    { 0.23277111,  0.81925215},
    {-0.04367417,  0.30952061},
    {-0.4931112 ,  0.55145933},
    {-0.78287756, -0.40832679},
    {-0.45266041,  0.26767449},
    {-0.19905406, -0.02066478},
    {-0.00670141,  0.51514763},
    { 0.14742235, -0.84160096},
    { 0.05853309,  0.57925968},
    { 0.05116486, -0.41123827},
    { 0.0063878 ,  0.72016596},
    {-0.32452109, -0.89742536},
    { 0.53531466, -0.32224962},
    {-0.05138841, -0.62892213},
    {-0.87586551,  0.0030228 },
    { 0.91015664, -0.04491546},
    {-0.08270389, -0.74938649},
    {-0.60847032, -0.19709423},
    { 0.69280502, -0.24633194},
    {-0.00203186, -0.95271331},
    {-0.75505301, -0.45141735},
    { 0.35192106, -0.87774144},
    {-0.47432603, -0.63489716},
    { 0.5806755 ,  0.28115667},
    { 0.6934058 , -0.48463034},
    { 0.30522932,  0.24507918},
    {-0.16404957,  0.18318721},
    {-0.55109203, -0.4667833 },
    {-0.38212837, -0.13039155},
    {-0.76455652, -0.58506595},
    {-0.05517833, -0.58396479},
    {-0.29708975,  0.62018128},
    { 0.52109725,  0.36880081},
    {-0.16793336, -0.82001413},
    { 0.159529  , -0.29196939},
    {-0.40267215,  0.70380431}
};
    

float LinearDepth(float depth, float zNear, float zFar){
    float zDis = zFar - zNear;

    return (2.f*depth*zDis) / (zFar+zNear-depth*zDis);
}

float PCF(int lightIndex, vec3 fragPos){
    float ocllusion = 0.f;

    vec4 shadowCoord = biasMat * ubo1.lights[lightIndex].shadowViewProj * vec4(fragPos, 1.f);
    shadowCoord.xyz = shadowCoord.xyz / shadowCoord.w;
    shadowCoord.y = 1.f - shadowCoord.y;

    float shadowDepth = texture(shadowMaps[ubo1.lights[lightIndex].shadowMapIndex], shadowCoord.xy).r;
    //float linearShadowDepth = LinearDepth(shadowDepth, ubo0.lights[lightIndex].cameraZnear, ubo0.lights[lightIndex].cameraZfar);
        
    for(int i=-3;i<=3;i+=1){
        for(int j=-3;j<=3;j+=1){
            //float shadowDepth = texture(shadowMaps[ubo1.lights[lightIndex].shadowMapIndex], shadowCoord.xy + vec2(i,j)/vec2(ubo0.ResolusionWidth, ubo0.ResolusionHeight)).r;
            //if ((shadowDepth+DepthBias) < shadowCoord.z) ocllusion+=1.f * gaussian_filter[i+3][j+3];

            float shadowDepth = texture(shadowMaps[ubo1.lights[lightIndex].shadowMapIndex], shadowCoord.xy + (vec2(5.f,5.f) * poisson_points[i*7+j+24]) / vec2(ubo0.ResolusionWidth, ubo0.ResolusionHeight)).r;
            if ((shadowDepth+DepthBias) < shadowCoord.z) ocllusion+=1.f;
        }
    }

    ocllusion /= 49.f;
    return ocllusion;
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, float metallic)
{
	vec3 F0 = mix(vec3(0.04), vec3(1.f), metallic); // * material.specular
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;    
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(vec3 diffuseColor, vec3 lightColor, vec3 L, vec3 V, vec3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed
	//vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, metallic);

		vec3 spec = D * F * G / ((4.0 * dotNL * dotNV) + 0.0001);
        vec3 diff = (1.f - F) * (1.f - metallic) * diffuseColor / PI;

		color += (spec + diff) * dotNL * lightColor;
	}

	return color;
}

vec3 rgb2srgb(vec3 color){
    color.x = color.x<0.0031308 ? color.x*12.92f : pow(color.x, 1.0/2.4) * 1.055f - 0.055f;
    color.y = color.y<0.0031308 ? color.y*12.92f : pow(color.y, 1.0/2.4) * 1.055f - 0.055f;
    color.z = color.z<0.0031308 ? color.z*12.92f : pow(color.z, 1.0/2.4) * 1.055f - 0.055f;

    return color;
}

void main(){
    vec4 gBufferValue0 = texture(gBufferNormal, texCoord);
    vec4 gBufferValue1 = texture(gBufferPosition, texCoord);
    vec4 gBufferValue2 = texture(gAlbedo, texCoord);
    vec4 gBufferValue3 = texture(gMetallicAndRoughness, texCoord);
    vec4 gBufferValue4 = texture(gMatId, texCoord);

    int matId = int(gBufferValue4.r);
    vec3 fragNormal = gBufferValue0.rgb;
    vec3 fragPos = gBufferValue1.rgb;
    vec2 fragUV = vec2(gBufferValue0.a, gBufferValue1.a);
    vec4 fragAlbedo = gBufferValue2.rgba;
    float metalness = gBufferValue3.b;
    float roughness = gBufferValue3.g;
    
    vec3 fragcolor = vec3(0.f, 0.f, 0.f);

    for(int i=0;i< ubo1.lightNum;i++){
        float ocllusion = PCF(i, fragPos);

        vec3 L = -ubo1.lights[i].direction;
        vec3 V = ubo1.cameraPos.xyz - fragPos;
        V = normalize(V);

        {
            fragcolor += (1.f - ocllusion) * BRDF(fragAlbedo.rgb, ubo1.lights[i].intensity * ubo1.lights[i].color, L, V, fragNormal, metalness, roughness);
            fragcolor += fragAlbedo.rgb * 0.04f; //ambient
        }
    }

    color = vec4(fragcolor, 1.f);
}
