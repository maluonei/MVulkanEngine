#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 3) uniform sampler2D textures[1024];

layout(location = 0)out vec4 Normal;
layout(location = 1)out vec4 Position;
layout(location = 2)out vec4 Albedo;
layout(location = 3)out vec4 MetallicAndRoughness;
layout(location = 4)out uvec4 MatId;

layout(location = 0)in vec3 normal;
layout(location = 1)in vec3 worldPos;
layout(location = 2)in vec2 texCoord;
layout(location = 3)in flat int instanceId;

layout(std140, binding = 1) uniform TextureBuffer{
	int diffuseTextureIdx;
	int metallicAndRoughnessTextureIdx;
	int matId;
	int padding2;
} ubo1[256];

layout(std140, binding = 2) uniform TestBufferArray{
	vec4 testValue;
} ubo2[4];

void main() {
	int diffuseTextureIdx = ubo1[instanceId].diffuseTextureIdx;
	int metallicAndRoughnessTextureIdx = ubo1[instanceId].metallicAndRoughnessTextureIdx;

	Normal = vec4(normal, texCoord.x);
	Position = vec4(worldPos, texCoord.y);
	if(diffuseTextureIdx!=-1)
		Albedo = texture(textures[diffuseTextureIdx], texCoord);
	else
		Albedo = vec4(1.f);

	if(metallicAndRoughnessTextureIdx!=-1)
		MetallicAndRoughness.rgb = texture(textures[metallicAndRoughnessTextureIdx], texCoord).rgb;
	else
		MetallicAndRoughness.rgb = vec3(0.f, 0.5f, 0.5f);

	MatId.r = ubo1[instanceId].matId;
}
