#version 450
layout(location = 0)in vec3 Position;
layout(location = 1)in vec3 Normal;
layout(location = 2)in vec2 Coord;

layout(binding = 0) uniform UBO
{
	mat4 depthMVP;
} ubo;

void main()
{
	gl_Position = ubo.depthMVP * vec4(Position, 1.0);
}