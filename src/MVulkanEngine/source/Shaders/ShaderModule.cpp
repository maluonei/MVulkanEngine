#include "Shaders/ShaderModule.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"

ShaderModule::ShaderModule(
	const std::string& vertPath, 
	const std::string& fragPath, 
	std::string vertEntry,
	std::string fragEntry,
	bool compileEveryTime)
	: m_vertPath(vertPath),
	m_fragPath(fragPath), 
	m_vertEntry(vertEntry),
	m_fragEntry(fragEntry),
	m_compileEveryTime(compileEveryTime)
{
	load();
}

ShaderModule::ShaderModule(
	const std::string& vertPath, 
	const std::string& geomPath, 
	const std::string& fragPath, 
	std::string vertEntry, 
	std::string geomEntry, 
	std::string fragEntry, 
	bool compileEveryTime)
	: m_vertPath(vertPath),
	m_fragPath(fragPath),
	m_geomPath(geomPath),
	m_vertEntry(vertEntry),
	m_fragEntry(fragEntry),
	m_geomEntry(geomEntry),
	m_compileEveryTime(compileEveryTime)
{
	load();
}

void ShaderModule::Create(VkDevice device)
{
	m_vertShader.Create(device);
	m_fragShader.Create(device);
	if (UseGeometryShader()) {
		m_geomShader.Create(device);
	}
}

void ShaderModule::Clean()
{
	m_vertShader.Clean();
	m_fragShader.Clean();
	if (UseGeometryShader()) {
		m_geomShader.Clean();
	}
}

size_t ShaderModule::GetBufferSizeBinding(uint32_t binding) const
{
	return size_t(0);
}

void ShaderModule::SetUBO(uint8_t binding, void* data)
{

}

void* ShaderModule::GetData(uint32_t binding, uint32_t index)
{
	return nullptr;
}

void ShaderModule::load()
{
	m_vertShader = MVulkanShader(m_vertPath, ShaderStageFlagBits::VERTEX, m_vertEntry, m_compileEveryTime);
	m_fragShader = MVulkanShader(m_fragPath, ShaderStageFlagBits::FRAGMENT, m_fragEntry, m_compileEveryTime);
	if(m_geomPath.size() > 0)
		m_geomShader = MVulkanShader(m_geomPath, ShaderStageFlagBits::GEOMETRY, m_geomEntry, m_compileEveryTime);
}


TestShader::TestShader():ShaderModule("glsl/test.vet.glsl", "glsl/test.frag.glsl")
{
	sizes.resize(3);
	sizes[0] = sizeof(ubo0);
	sizes[1] = sizeof(ubo1);
	sizes[2] = 0;
}

void TestShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
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

void PhongShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
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

void GbufferShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
		ubo0 = *reinterpret_cast<UniformBufferObject0*>(data);
		return;
	case 1:
		ubo1 = *reinterpret_cast<UniformBufferObject1*>(data);
		return;
	}
}

void* GbufferShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	case 1:
		return (void*)&ubo1;
	}
}

GbufferShader::UniformBufferObject1 GbufferShader::GetFromScene(const std::shared_ptr<Scene> m_scene) {
	GbufferShader::UniformBufferObject1 ubo1;

	//auto meshNames = m_scene->GetMeshNames();
	auto numPrims = m_scene->GetNumPrimInfos();
	auto drawIndexedIndirectCommands = m_scene->GetIndirectDrawCommands();

	for (auto i = 0; i < numPrims; i++) {
		//auto name = meshNames[i];
		//auto mesh = m_scene->GetMesh(i);
		auto primInfo = m_scene->m_primInfos[i];
		auto mat = m_scene->GetMaterial(primInfo.material_id);

		auto indirectCommand = drawIndexedIndirectCommands[i];
		if (mat->diffuseTexture != "") {
			auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
			ubo1.ubo[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
		}
		else {
			ubo1.ubo[indirectCommand.firstInstance].diffuseTextureIdx = -1;
			ubo1.ubo[indirectCommand.firstInstance].diffuseColor = glm::vec3(mat->diffuseColor.r, mat->diffuseColor.g, mat->diffuseColor.b);
		}

		if (mat->metallicAndRoughnessTexture != "") {
			auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
			ubo1.ubo[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = metallicAndRoughnessTexId;
		}
		else {
			ubo1.ubo[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = -1;
		}


		if (mat->normalMap != "") {
			auto normalmapIdx = Singleton<TextureManager>::instance().GetTextureId(mat->normalMap);
			ubo1.ubo[indirectCommand.firstInstance].normalMapIdx = normalmapIdx;
		}
		else {
			ubo1.ubo[indirectCommand.firstInstance].normalMapIdx = -1;
		}
	}

	return ubo1;
	//m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
}

GbufferShader2::GbufferShader2() :
	//ShaderModule("gbuffer.vert.glsl", "gbuffer.frag.glsl")
	ShaderModule("hlsl/gbuffer.vert.hlsl", "hlsl/gbuffer/gbuffer.frag.hlsl")
{
}

size_t GbufferShader2::GetBufferSizeBinding(uint32_t binding) const
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

void GbufferShader2::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
		ubo0 = *reinterpret_cast<UniformBufferObject0*>(data);
		return;
	case 1:
		ubo1 = *reinterpret_cast<UniformBufferObject1*>(data);
		return;
	}
}

void* GbufferShader2::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	case 1:
		return (void*)&ubo1;
	}
}

GbufferShader2::UniformBufferObject1 GbufferShader2::GetFromScene(const std::shared_ptr<Scene> m_scene) {
	GbufferShader2::UniformBufferObject1 ubo1;

	//auto meshNames = m_scene->GetMeshNames();
	auto numPrims = m_scene->GetNumPrimInfos();
	auto drawIndexedIndirectCommands = m_scene->GetIndirectDrawCommands();

	for (auto i = 0; i < numPrims; i++) {
		//auto name = meshNames[i];
		//auto mesh = m_scene->GetMesh(i);
		auto primInfo = m_scene->m_primInfos[i];
		auto mat = m_scene->GetMaterial(primInfo.material_id);

		auto indirectCommand = drawIndexedIndirectCommands[i];
		if (mat->diffuseTexture != "") {
			auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
			ubo1.ubo[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
		}
		else {
			ubo1.ubo[indirectCommand.firstInstance].diffuseTextureIdx = -1;
			ubo1.ubo[indirectCommand.firstInstance].diffuseColor = glm::vec3(mat->diffuseColor.r, mat->diffuseColor.g, mat->diffuseColor.b);
		}

		if (mat->metallicAndRoughnessTexture != "") {
			auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
			ubo1.ubo[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = metallicAndRoughnessTexId;
		}
		else {
			ubo1.ubo[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = -1;
		}


		if (mat->normalMap != "") {
			auto normalmapIdx = Singleton<TextureManager>::instance().GetTextureId(mat->normalMap);
			ubo1.ubo[indirectCommand.firstInstance].normalMapIdx = normalmapIdx;
		}
		else {
			ubo1.ubo[indirectCommand.firstInstance].normalMapIdx = -1;
		}
	}

	return ubo1;
	//m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
}

SquadPhongShader::SquadPhongShader():ShaderModule("glsl/phong_sqad.vert.glsl", "glsl/phong_sqad.frag.glsl")
{
}

size_t SquadPhongShader::GetBufferSizeBinding(uint32_t binding) const
{
	return sizeof(DirectionalLightBuffer);
}

void SquadPhongShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
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

void ShadowShader::SetUBO(uint8_t binding, void* data)
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

void LightingPbrShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
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

LightingPbrShader2::LightingPbrShader2() :ShaderModule("hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr_packed.frag.hlsl")
{

}

size_t LightingPbrShader2::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(LightingPbrShader2::UniformBuffer0);
	}
	//return sizeof(DirectionalLightBuffer);
}

void LightingPbrShader2::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
		ubo0 = *reinterpret_cast<LightingPbrShader2::UniformBuffer0*>(data);
		return;
	}
}

void* LightingPbrShader2::GetData(uint32_t binding, uint32_t index)
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

void LightingIBLShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
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

void SkyboxShader::SetUBO(uint8_t binding, void* data)
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

void IrradianceConvolutionShader::SetUBO(uint8_t binding, void* data)
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

void PreFilterEnvmapShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
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

void IBLBrdfShader::SetUBO(uint8_t binding, void* data)
{
	
}

void* IBLBrdfShader::GetData(uint32_t binding, uint32_t index)
{
	return nullptr;
}

ComputeShaderModule::ComputeShaderModule(
	const std::string& compPath, 
	std::string compEntry, 
	bool compileEveryTime):
	m_compPath(compPath),
	m_entryPoint(compEntry),
	m_compileEveryTime(compileEveryTime)
{
	load();
}

void ComputeShaderModule::Create(VkDevice device)
{
	m_compShader.Create(device);
}

void ComputeShaderModule::Clean()
{
	m_compShader.Clean();
}

size_t ComputeShaderModule::GetBufferSizeBinding(uint32_t binding) const
{
	return size_t();
}

void ComputeShaderModule::SetUBO(uint8_t binding, void* data)
{
}

void* ComputeShaderModule::GetData(uint32_t binding, uint32_t index)
{
	return nullptr;
}

void ComputeShaderModule::load()
{
	m_compShader = MVulkanShader(m_compPath, ShaderStageFlagBits::COMPUTE, m_entryPoint, m_compileEveryTime);
}

TestCompShader::TestCompShader() :ComputeShaderModule("hlsl/test.comp.hlsl")
{
}

size_t TestCompShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(TestCompShader::Constants);
	case 1:
		return sizeof(TestCompShader::InputBuffer);
	case 2:
		return sizeof(TestCompShader::OutputBuffer);
	}
}

void TestCompShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
	{
		constant0 = *reinterpret_cast<TestCompShader::Constants*>(data);
		break;
	}
	case 1:
	{
		input1 = *reinterpret_cast<TestCompShader::InputBuffer*>(data);
		break;
	}
	case 2:
	{
		output2 = *reinterpret_cast<TestCompShader::OutputBuffer*>(data);
		break;
	}
	}
}

void* TestCompShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
		case 0: return reinterpret_cast<void*>(&constant0);
		case 1: return reinterpret_cast<void*>(&input1);
		case 2: return reinterpret_cast<void*>(&output2);
	}
}

TestSquadShader::TestSquadShader() :ShaderModule("hlsl/test_squad.vert.hlsl", "hlsl/test_squad.frag.hlsl")
{
}


LightingPbrSSRShader::LightingPbrSSRShader() :ShaderModule("hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbrssr.frag.hlsl")
{

}

size_t LightingPbrSSRShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(LightingPbrSSRShader::UniformBuffer0);
	}
	//return sizeof(DirectionalLightBuffer);
}

void LightingPbrSSRShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
		ubo0 = *reinterpret_cast<LightingPbrSSRShader::UniformBuffer0*>(data);
		return;
	}
}

void* LightingPbrSSRShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	}
}

SSRShader::SSRShader() :ShaderModule("hlsl/lighting_pbr.vert.hlsl", "hlsl/ssr.frag.hlsl")
{

}

size_t SSRShader::GetBufferSizeBinding(uint32_t binding) const
{
	switch (binding) {
	case 0:
		return sizeof(SSRShader::UniformBuffer0);
	}
}

void SSRShader::SetUBO(uint8_t binding, void* data)
{
	switch (binding) {
	case 0:
		ubo0 = *reinterpret_cast<SSRShader::UniformBuffer0*>(data);
		return;
	}
}

void* SSRShader::GetData(uint32_t binding, uint32_t index)
{
	switch (binding) {
	case 0:
		return (void*)&ubo0;
	}
}

MeshShaderModule::MeshShaderModule(
	const std::string& taskPath, const std::string& meshPath, const std::string& fragPath, 
	std::string taskEntry, std::string meshEntry, std::string fragEntry, bool compileEveryTime)
	: 
	m_taskPath(taskPath),
	m_meshPath(meshPath),
	m_fragPath(fragPath),
	m_taskEntry(taskEntry),
	m_meshEntry(meshEntry),
	m_fragEntry(fragEntry),
	m_compileEveryTime(compileEveryTime)
{
	load();
}

MeshShaderModule::MeshShaderModule(const std::string& meshPath, const std::string& fragPath, std::string meshEntry, std::string fragEntry, bool compileEveryTime)
	:MeshShaderModule("", meshPath, fragPath, "", meshEntry, fragEntry, compileEveryTime)
{
}

void MeshShaderModule::Create(VkDevice device)
{
	if(UseTaskShader())
		m_taskShader.Create(device);
	m_meshShader.Create(device);
	m_fragShader.Create(device);
}

void MeshShaderModule::Clean()
{
	m_meshShader.Clean();
	m_fragShader.Clean();
	if (UseTaskShader()) {
		m_taskShader.Clean();
	}
}

size_t MeshShaderModule::GetBufferSizeBinding(uint32_t binding) const
{
	return size_t(0);
}

void MeshShaderModule::SetUBO(uint8_t binding, void* data)
{
}

void* MeshShaderModule::GetData(uint32_t binding, uint32_t index)
{
	return nullptr;
}

void MeshShaderModule::load()
{
	m_meshShader = MVulkanShader(m_meshPath, ShaderStageFlagBits::MESH, m_meshEntry, m_compileEveryTime);
	m_fragShader = MVulkanShader(m_fragPath, ShaderStageFlagBits::FRAGMENT, m_fragEntry, m_compileEveryTime);
	if (m_taskPath.size() > 0)
		m_taskShader = MVulkanShader(m_taskPath, ShaderStageFlagBits::TASK, m_taskEntry, m_compileEveryTime);
}