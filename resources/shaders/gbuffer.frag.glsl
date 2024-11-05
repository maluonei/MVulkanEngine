#version 450 core

layout(location = 0)out vec4 Normal;
layout(location = 1)out vec2 TexCoord;

layout(location = 0)in vec3 normal;
layout(location = 1)in vec2 texCoord;

void main() {
	Normal = vec4(normal, 1.f);
	TexCoord = texCoord;
}
