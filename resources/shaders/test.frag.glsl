#version 450 core

layout(location = 0)out vec4 color;

layout(std140, binding = 1) uniform UniformBufferObject{
	vec3 color0;
	float padding0;
	vec3 color1;
	float padding1;
} ubo;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0)in vec2 texCoord;

void main(){
	//color = vec4(1.f, 0.f, 0.f, 1.f);
	//color = vec4(ubo.color0 + ubo.color1, 1.0f);
	//color = vec4(texCoord.x, texCoord.y, 0.f, 1.f);
	color = texture(texSampler, texCoord);
}

//#version 450
//
//layout(location = 0) in vec3 fragColor;
//
//layout(location = 0) out vec4 outColor;
//
//void main() {
//    outColor = vec4(fragColor, 1.0);
//}
//