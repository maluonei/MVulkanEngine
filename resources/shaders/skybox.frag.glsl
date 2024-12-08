#version 450 core

layout (binding = 1) uniform samplerCube samplerEnv;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outColor;

void main() 
{
	//vec3 color = texture(samplerEnv, inUVW).rgb;
	
	//outColor = vec4(color, 1.0);
	outColor = vec4(1.f, 0.f, 0.f, 1.f);
}