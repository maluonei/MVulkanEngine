#version 450 core

layout(binding = 1) uniform sampler2D samplerColor;

layout(location = 0)out vec4 Normal;
layout(location = 1)out vec4 Position;
layout(location = 2)out vec4 Albedo;

layout(location = 0)in vec3 normal;
layout(location = 1)in vec3 worldPos;
layout(location = 2)in vec2 texCoord;

void main() {
	Normal = vec4(normal, texCoord.x);
	Position = vec4(worldPos, texCoord.y);
	Albedo = texture(samplerColor, texCoord);
}
