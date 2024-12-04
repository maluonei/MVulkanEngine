#version 450 core
#extension GL_NV_uniform_buffer_std430_layout : enable
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0)out vec4 color;

layout(binding = 2) uniform sampler2D gBufferNormal;
layout(binding = 3) uniform sampler2D gBufferPosition;
layout(binding = 4) uniform sampler2D gAlbedo;
layout(binding = 5) uniform sampler2D gMetallicAndRoughness;
layout(binding = 6) uniform sampler2D shadowMaps[2];

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
        
    for(int i=-3;i<=3;i++){
        for(int j=-3;j<=3;j++){
            float shadowDepth = texture(shadowMaps[ubo1.lights[lightIndex].shadowMapIndex], shadowCoord.xy + vec2(i,j)/vec2(ubo0.ResolusionWidth, ubo0.ResolusionHeight)).r;
            if ((shadowDepth+DepthBias) < shadowCoord.z) ocllusion+=1.;
        }
    }

    ocllusion/=49.f;

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

vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed
	vec3 lightColor = vec3(1.0);

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

		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
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

    vec3 fragNormal = gBufferValue0.rgb;
    vec3 fragPos = gBufferValue1.rgb;
    vec2 fragUV = vec2(gBufferValue0.a, gBufferValue1.a);
    vec4 fragAlbedo = gBufferValue2.rgba;
    float metalness = gBufferValue3.b;
    float roughness = gBufferValue3.g;
    
    vec3 fragcolor = vec3(0.f, 0.f, 0.f);

    for(int i=0;i< ubo1.lightNum;i++){
        //float shadow = 1.0;
        //vec4 shadowCoord = biasMat * ubo1.lights[i].shadowViewProj * vec4(fragPos, 1.0);
        //shadowCoord.xyz = shadowCoord.xyz / shadowCoord.w;
        //shadowCoord.y = 1.f - shadowCoord.y;
        //
        //float shadowDepth = 0.f;
        //if (shadowCoord.x < 0 || shadowCoord.x>1. || shadowCoord.y < 0 || shadowCoord.y>1.) {
        //    shadow = 0.f;
        //}
        //else {
        //  shadowDepth = texture(shadowMaps[ubo1.lights[i].shadowMapIndex], shadowCoord.xy).r;
        //  if ((shadowDepth+DepthBias) < shadowCoord.z) shadow = 0.f;
        //}
        float ocllusion = PCF(i, fragPos);

        vec3 L = -ubo1.lights[i].direction;
        vec3 V = ubo1.cameraPos.xyz - fragPos;
        V = normalize(V);

        {
            //ocllusion = 0.f;
            //fragcolor += shadow * ubo1.lights[i].intensity * ubo1.lights[i].color * fragAlbedo.rgb * BRDF(L, V, fragNormal, metalness, roughness);
            fragcolor += (1.f - ocllusion) * ubo1.lights[i].intensity * ubo1.lights[i].color * fragAlbedo.rgb * BRDF(L, V, fragNormal, metalness, roughness);
            fragcolor += fragAlbedo.rgb * 0.01; //ambient
        }
    }

    fragcolor = rgb2srgb(fragcolor);
    color = vec4(fragcolor, 1.f);
}
