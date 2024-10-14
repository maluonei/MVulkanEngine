#version 450 core

layout(location = 0)in vec3 Position;
layout(location = 1)in vec3 Normal;
layout(location = 2)in vec2 Coord;

layout(location = 0)out vec2 texCoord;

//layout(binding = 0) uniform UniformBufferObject{
//	mat4 Model;
//	mat4 View;
//	mat4 Projection;
//} ubo;

void main(){
	texCoord = Coord;
	//vec4 worldSpacePosition = ubo.Model * vec4(Position, 1.f);
	//vec4 screenSpacePosition = ubo.Projection * ubo.View * worldSpacePosition;

	//gl_Position = screenSpacePosition;
	gl_Position = vec4(Position, 1.f);
}


//#version 450
//
//layout(location = 0) out vec3 fragColor;
//
//vec2 positions[3] = vec2[](
//    vec2(0.0, -0.5),
//    vec2(0.5, 0.5),
//    vec2(-0.5, 0.5)
//    );
//
//vec3 colors[3] = vec3[](
//    vec3(1.0, 0.0, 0.0),
//    vec3(0.0, 1.0, 0.0),
//    vec3(0.0, 0.0, 1.0)
//    );
//
//void main() {
//    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
//    fragColor = colors[gl_VertexIndex];
//}

