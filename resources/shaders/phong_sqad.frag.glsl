#version 450 core

layout(location = 0)out vec4 color;

layout(binding = 0) uniform sampler2D gBuffer0;
layout(binding = 1) uniform sampler2D gBuffer1;

layout(location = 0)in vec2 texCoord;

void main(){
    vec3 normal = texture(gBuffer0, texCoord).rgb;
    vec2 uv =  texture(gBuffer1, texCoord).rg;
    
    color = vec4(normal*0.5+0.5, 1.f);
    //color = vec4(uv, 0.f, 1.f);
    //color = vec4(0.f, 1.f, 0.5f, 1.f);
}
