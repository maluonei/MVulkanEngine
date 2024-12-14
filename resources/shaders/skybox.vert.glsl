#version 450 core

layout(location = 0)in vec3 Position;
layout(location = 1)in vec3 Normal;
layout(location = 2)in vec2 Coord;

layout (location = 0) out vec3 outUVW;

layout(std140, binding = 0) uniform UniformBufferObject{
	mat4 View;
	mat4 Projection;
	//bool flipY;
	//float padding0;
	//float padding1;
	//float padding2;
} ubo0;

void main() {
	outUVW = Position;
	vec4 pos = ubo0.Projection * mat4(mat3(ubo0.View)) * vec4(Position.xyz, 1.0);
	gl_Position = pos.xyww;
	//if(ubo0.flipY){
	//	gl_Position.y = -gl_Position.y;
	//}
}