#include "Shaders/ShaderModule.hpp"

ShaderModule::ShaderModule(const std::string& vertPath, const std::string& fragPath)
	: m_vertPath(vertPath), m_fragPath(fragPath)
{
	load();
}

void ShaderModule::Create(VkDevice device)
{
	m_vertShader.Create(device);
	m_fragShader.Create(device);
}

void ShaderModule::Clean()
{
	m_vertShader.Clean();
	m_fragShader.Clean();
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
	m_vertShader = MVulkanShader(m_vertPath, ShaderStageFlagBits::VERTEX);
	m_fragShader = MVulkanShader(m_fragPath, ShaderStageFlagBits::FRAGMENT);
}


TestShader::TestShader():ShaderModule("glsl/test.vet.glsl", "glsl/test.frag.glsl")
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
		return (void*)&ubo0;
	case 1:
		return (void*)&ubo1;
	case 2:
	default:
		return nullptr;
	}
}

PhongShader::PhongShader()
:ShaderModule("glsl/phong.vert.glsl", "glsl/phong.frag.glsl"){
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
		return (void*)&ubo0;
	case 1:
		return (void*)&ubo1;
	case 2:
	default:
		return nullptr;
	}
}

GbufferShader::GbufferShader():
	//ShaderModule("gbuffer.vert.glsl", "gbuffer.frag.glsl")
	ShaderModule("hlsl/gbuffer.vert.hlsl", "hlsl/gbuffer.frag.hlsl")
{
}

size_t GbufferShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(UniformBufferObject0);
	case 1:
		return sizeof(UniformBufferObject1);
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
	}
}

void* GbufferShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	case 1:
		return (void*)&(ubo1[index]);
	}
}

SquadPhongShader::SquadPhongShader():ShaderModule("glsl/phong_sqad.vert.glsl", "glsl/phong_sqad.frag.glsl")
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
	return (void*)&ubo0;
}

ShadowShader::ShadowShader():ShaderModule("glsl/shadow.vert.glsl", "glsl/shadow.frag.glsl")
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
	return (void*)&ubo0;
}

LightingPbrShader::LightingPbrShader() :ShaderModule("hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr.frag.hlsl")
{

}

size_t LightingPbrShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(LightingPbrShader::UniformBuffer0);
	}
	//return sizeof(DirectionalLightBuffer);
}

void LightingPbrShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<LightingPbrShader::UniformBuffer0*>(data);
		return;
	}
}

void* LightingPbrShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	}
}


LightingIBLShader::LightingIBLShader() :ShaderModule("hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_ibl.frag.hlsl")
{

}

size_t LightingIBLShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(LightingIBLShader::UniformBuffer0);
	}
}

void LightingIBLShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<LightingIBLShader::UniformBuffer0*>(data);
		return;
	}
}

void* LightingIBLShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	}
}

SkyboxShader::SkyboxShader() :ShaderModule("glsl/skybox.vert.glsl", "glsl/skybox.frag.glsl")
{

}

size_t SkyboxShader::GetBufferSizeBinding(uint32_t binding) const
{
	return sizeof(SkyboxShader::UniformBuffer0);
}

void SkyboxShader::SetUBO(uint8_t index, void* data)
{
	ubo0 = *reinterpret_cast<SkyboxShader::UniformBuffer0*>(data);
}

void* SkyboxShader::GetData(uint32_t binding, uint32_t index)
{
	return (void*)&ubo0;
}

IrradianceConvolutionShader::IrradianceConvolutionShader() :ShaderModule("glsl/skybox.vert.glsl", "glsl/convolution_irradiance.frag.glsl")
{

}

size_t IrradianceConvolutionShader::GetBufferSizeBinding(uint32_t binding) const
{
	return sizeof(IrradianceConvolutionShader::UniformBuffer0);
}

void IrradianceConvolutionShader::SetUBO(uint8_t index, void* data)
{
	ubo0 = *reinterpret_cast<IrradianceConvolutionShader::UniformBuffer0*>(data);
}

void* IrradianceConvolutionShader::GetData(uint32_t binding, uint32_t index)
{
	return (void*)&ubo0;
}

PreFilterEnvmapShader::PreFilterEnvmapShader() :ShaderModule("glsl/skybox.vert.glsl", "glsl/prefilterSpecular.frag.glsl")
{

}

size_t PreFilterEnvmapShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(PreFilterEnvmapShader::UniformBuffer0);
	case 1:
		return sizeof(PreFilterEnvmapShader::UniformBuffer1);
	}
}

void PreFilterEnvmapShader::SetUBO(uint8_t index, void* data)
{
	switch (index) {
	case 0:
		ubo0 = *reinterpret_cast<PreFilterEnvmapShader::UniformBuffer0*>(data);
		break;
	case 1:
		ubo1 = *reinterpret_cast<PreFilterEnvmapShader::UniformBuffer1*>(data);
		break;
	}
}

void* PreFilterEnvmapShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	case 1:
		return (void*)&ubo1;
	}
}

IBLBrdfShader::IBLBrdfShader() :ShaderModule("glsl/IBLbrdf.vert.glsl", "glsl/IBLbrdf.frag.glsl")
{

}

size_t IBLBrdfShader::GetBufferSizeBinding(uint32_t binding) const
{
	return 0;
}

void IBLBrdfShader::SetUBO(uint8_t index, void* data)
{
	
}

void* IBLBrdfShader::GetData(uint32_t binding, uint32_t index)
{
	return nullptr;
}