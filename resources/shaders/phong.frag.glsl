#version 450 core

layout(location = 0)out vec4 color;

layout(std140, binding = 1) uniform UniformBufferObject{
	vec3 cameraPosition;
	float padding0;
} ubo1;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0)in vec2 texCoord;
layout(location = 1)in vec3 worldPosition;
layout(location = 2)in vec3 normal;

vec3 lightPosition = vec3(2.f, 2.f, 0.f);
vec3 lightColor = vec3(1.f, 1.f, 1.f);

void main() {
	vec3 n = normalize(normal);

	vec3 view = normalize(ubo1.cameraPosition - worldPosition);
	vec3 lightDir = normalize(lightPosition - worldPosition);

	vec3 H = normalize(lightDir + view);

	vec3 ambient = vec3(0.1f);
	vec3 diffuse = lightColor * max(0, dot(lightDir, n));
	vec3 specular = lightColor * max(0, pow(dot(n, H), 20));
	
	color = vec4(ambient + diffuse + specular, 1.f);
}