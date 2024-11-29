#version 450 core

layout(location = 0)out vec4 color;

layout(binding = 1) uniform sampler2D gBufferNormal;
layout(binding = 2) uniform sampler2D gBufferPosition;
layout(binding = 3) uniform sampler2D gAlbedo;
layout(binding = 4) uniform sampler2D shadowMaps[2];

layout(location = 0)in vec2 texCoord;

struct Light {
    vec3 direction;
    float intensity;
    vec3 color;
    int shadowMapIndex;
    mat4 shadowViewProj;
};

layout(std140, binding = 0) uniform DirectionalLightBuffer{
    Light lights[2];
    vec3 cameraPos;
    int lightNum;
} ubo0;

//float LinearizeDepth(float depth)
//{
//    float n = ubo.zNear;
//    float f = ubo.zFar;
//    float z = depth;
//    return (2.0 * n) / (f + n - z * (f - n));
//}

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0);

const float offset = 0.001f;

void main(){
    vec4 gBufferValue0 = texture(gBufferNormal, texCoord);
    vec4 gBufferValue1 = texture(gBufferPosition, texCoord);
    vec4 gBufferValue2 = texture(gAlbedo, texCoord);
    
    vec3 fragNormal = gBufferValue0.rgb;
    vec3 fragPos = gBufferValue1.rgb;
    vec2 fragUV = vec2(gBufferValue0.a, gBufferValue1.a);
    vec4 fragAlbedo = gBufferValue2.rgba;
    
    vec3 fragcolor = vec3(0.f, 0.f, 0.f);
    
    for(int i=0;i< ubo0.lightNum;i++){
        float shadow = 1.0;
        vec4 shadowCoord = biasMat * ubo0.lights[i].shadowViewProj * vec4(fragPos, 1.0);
        shadowCoord.xyz = shadowCoord.xyz / shadowCoord.w;
        shadowCoord.y = 1.f - shadowCoord.y;

        float shadowDepth = 0.f;
        if (shadowCoord.x < 0 || shadowCoord.x>1. || shadowCoord.y < 0 || shadowCoord.y>1.) {
            shadow = 0.f;
        }
        else {
          shadowDepth = texture(shadowMaps[ubo0.lights[i].shadowMapIndex], shadowCoord.xy).r;
          if ((shadowDepth+offset) < shadowCoord.z) shadow = 0.f;
        }


        // Vector to light
        vec3 L = -ubo0.lights[i].direction;

        // Viewer to fragment
        vec3 V = ubo0.cameraPos.xyz - fragPos;
        V = normalize(V);

        //if(dist < ubo.lights[i].radius)
        {
            // Light to fragment
            L = normalize(L);

            // Attenuation
            float atten = 1.;

            // Diffuse part
            vec3 N = normalize(fragNormal);
            float NdotL = max(0.0, dot(N, L));
            vec3 diff = ubo0.lights[i].color * fragAlbedo.rgb * NdotL * atten;

            // Specular part
            // Specular map values are stored in alpha of albedo mrt
            vec3 R = reflect(-L, N);
            float NdotR = max(0.0, dot(R, V));
            vec3 spec = ubo0.lights[i].color * fragAlbedo.a * pow(NdotR, 16.0) * atten;

            vec3 ambient = 0.1 * fragAlbedo.rgb;

            fragcolor += shadow * (diff + spec) + ambient;

        }
    }

    color = vec4(fragcolor, 1.f);
}
