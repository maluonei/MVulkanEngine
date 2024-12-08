#version 450 core

layout(location = 0)in vec3 Position;
layout(location = 1)in vec3 Normal;
layout(location = 2)in vec2 Coord;

layout (location = 0) out vec3 outUVW;

layout(std140, binding = 0) uniform UniformBufferObject{
	mat4 View;
	mat4 Projection;
} ubo0;

void main() {
	outUVW = Position;
	vec4 pos = ubo0.Projection * ubo0.View * vec4(Position.xyz, 1.0);
	gl_Position = vec4(pos.xyz, 1.f);
	gl_Position = vec4(Position.xy, 0.5f, 1.f);
	//gl_Position = pos.xyww;
}