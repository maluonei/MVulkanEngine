#version 450 core

layout(location = 0)in vec3 Position;
layout(location = 1)in vec3 Normal;
layout(location = 2)in vec2 Coord;

layout(location = 0)out vec3 normal;
layout(location = 1)out vec3 worldPos;
layout(location = 2)out vec2 texCoord;

layout(std140, binding = 0) uniform UniformBufferObject{
	mat4 Model;
	mat4 View;
	mat4 Projection;
} ubo0;

void main() {
	texCoord = Coord;

	vec4 worldSpacePosition = ubo0.Model * vec4(Position, 1.f);
	vec4 screenSpacePosition = ubo0.Projection * ubo0.View * worldSpacePosition;
	//vec3 worldPosition = worldSpacePosition.xyz;

	mat3 normalMatrix = transpose(inverse(mat3(ubo0.Model)));
	normal = normalize(normalMatrix * Normal);
	texCoord = Coord;
	worldPos = vec3(worldSpacePosition);

	gl_Position = screenSpacePosition;
}