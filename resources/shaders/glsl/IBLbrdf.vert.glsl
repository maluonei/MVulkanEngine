#version 450 core

layout(location = 0)in vec3 Position;
layout(location = 1)in vec3 Normal;
layout(location = 2)in vec2 Coord;

layout (location = 0) out vec2 TexCoords;

void main() {
	gl_Position = vec4(Position, 1.0f);

	TexCoords = Coord;
}