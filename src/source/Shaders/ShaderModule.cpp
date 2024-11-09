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

void* ShaderModule::GetData(uint32_t binding)
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

void* TestShader::GetData(uint32_t binding)
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

//void PhongShader::SetUBO0(glm::mat4 model, glm::mat4 view, glm::mat4 projection)
//{
//	ubo0.Model = model;
//	ubo0.View = view;
//	ubo0.Projection = projection;
//}
//
//void PhongShader::SetUBO1(glm::vec3 cameraPosition)
//{
//	ubo1.cameraPosition = cameraPosition;
//}

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

void* PhongShader::GetData(uint32_t binding)
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
	return sizeof(UniformBufferObject);
}

void GbufferShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<UniformBufferObject*>(data);
		return;
	}
}

void* GbufferShader::GetData(uint32_t binding)
{
	return &ubo0;
}

SquadPhongShader::SquadPhongShader():ShaderModule("phong_sqad.vert.glsl", "phong_sqad.frag.glsl")
{
}
