#include "Shaders/ShaderModule.hpp"

ShaderModule::ShaderModule(const std::string& vertPath, const std::string& fragPath)
	: m_vertPath(vertPath), m_fragPath(fragPath)
{
	load();
}

void ShaderModule::Create(VkDevice device)
{
	vertShader.Create(device);
	fragShader.Create(device);
}

void ShaderModule::Clean(VkDevice device)
{
	vertShader.Clean(device);
	fragShader.Clean(device);
}

size_t ShaderModule::GetBufferSizeBinding(uint32_t binding) const
{
	return size_t(0);
}

void ShaderModule::SetUBO(uint8_t index, void* data)
{

}

void* ShaderModule::GetData(uint32_t binding, uint32_t index)
{
	return nullptr;
}

void ShaderModule::load()
{
	vertShader = MVulkanShader(m_vertPath, ShaderStageFlagBits::VERTEX);
	fragShader = MVulkanShader(m_fragPath, ShaderStageFlagBits::FRAGMENT);
}


TestShader::TestShader():ShaderModule("test.vet.glsl", "test.frag.glsl")
{
	sizes.resize(3);
	sizes[0] = sizeof(ubo0);
	sizes[1] = sizeof(ubo1);
	sizes[2] = 0;
}

void TestShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<UniformBufferObjectVert*>(data);
		return;
	case 1:
		ubo1 = *reinterpret_cast<UniformBufferObjectFrag*>(data);
		return;
	}
}

void* TestShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return &ubo0;
	case 1:
		return &ubo1;
	case 2:
	default:
		return nullptr;
	}
}

PhongShader::PhongShader()
:ShaderModule("phong.vert.glsl", "phong.frag.glsl"){
	sizes.resize(3);
	sizes[0] = sizeof(ubo0);
	sizes[1] = sizeof(ubo1);
	sizes[2] = 0;
}

void PhongShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<UniformBufferObjectVert*>(data);
		return;
	case 1:
		ubo1 = *reinterpret_cast<UniformBufferObjectFrag*>(data);
		return;
	}
}

void* PhongShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return &ubo0;
	case 1:
		return &ubo1;
	case 2:
	default:
		return nullptr;
	}
}

GbufferShader::GbufferShader():
	ShaderModule("gbuffer.vert.glsl", "gbuffer.frag.glsl")
{
}

size_t GbufferShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(UniformBufferObject0);
	case 1:
		return sizeof(UniformBufferObject1);
	case 2:
		return sizeof(UniformBufferObject2) * 4;
	default:
		return -1;
	}
}

void GbufferShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<UniformBufferObject0*>(data);
		return;
	case 1:
		for (auto i = 0; i < 256; i++) {
			ubo1[i] = reinterpret_cast<UniformBufferObject1*>(data)[i];
		}
		return;
	case 2:
		for (auto i = 0; i < 4; i++) {
			ubo2[i] = reinterpret_cast<UniformBufferObject2*>(data)[i];
		}
		return;
}
}

void* GbufferShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return &ubo0;
	case 1:
		return &(ubo1[index]);
	case 2:
		return &(ubo2[index]);
	}
}

SquadPhongShader::SquadPhongShader():ShaderModule("phong_sqad.vert.glsl", "phong_sqad.frag.glsl")
{
}

size_t SquadPhongShader::GetBufferSizeBinding(uint32_t binding) const
{
	return sizeof(DirectionalLightBuffer);
}

void SquadPhongShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<DirectionalLightBuffer*>(data);
		return;
	}
}

void* SquadPhongShader::GetData(uint32_t binding, uint32_t index)
{
	return &ubo0;
}

ShadowShader::ShadowShader():ShaderModule("shadow.vert.glsl", "shadow.frag.glsl")
{
}

size_t ShadowShader::GetBufferSizeBinding(uint32_t binding) const
{
	return sizeof(ShadowUniformBuffer);
}

void ShadowShader::SetUBO(uint8_t index, void* data)
{
	ubo0 = *reinterpret_cast<ShadowUniformBuffer*>(data);
}

void* ShadowShader::GetData(uint32_t binding, uint32_t index)
{
	return &ubo0;
}

LightingPbrShader::LightingPbrShader() :ShaderModule("lighting_pbr.vert.glsl", "lighting_pbr.frag.glsl")
{

}

size_t LightingPbrShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(LightingPbrShader::UniformBuffer0);
	case 1:
		return sizeof(LightingPbrShader::DirectionalLightBuffer);
	}
	//return sizeof(DirectionalLightBuffer);
}

void LightingPbrShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<LightingPbrShader::UniformBuffer0*>(data);
		return;
	case 1:
		ubo1 = *reinterpret_cast<LightingPbrShader::DirectionalLightBuffer*>(data);
		return;
	}
}

void* LightingPbrShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return &ubo0;
	case 1:
		return &ubo1;
	}
}