#version 450 core

layout(location = 0)out vec4 color;

layout(binding = 1) uniform sampler2D gBufferNormal;
layout(binding = 2) uniform sampler2D gBufferPosition;
layout(binding = 3) uniform sampler2D gAlbedo;

layout(location = 0)in vec2 texCoord;

struct Light {
    vec3 direction;
    float intensity;
    vec3 color;
    float padding0;
};

layout(std140, binding = 0) uniform DirectionalLightBuffer{
    Light lights[2];
    vec3 cameraPos;
    int lightNum;
} ubo0;

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

            fragcolor += diff + spec;

            fragcolor = fragAlbedo.rgb;
        }
    }

    color = vec4(fragcolor, 1.f);
    //color = vec4(normal*0.5+0.5, 1.f);
    //color = vec4(uv, 0.f, 1.f);
    //color = vec4(0.f, 1.f, 0.5f, 1.f);
}
