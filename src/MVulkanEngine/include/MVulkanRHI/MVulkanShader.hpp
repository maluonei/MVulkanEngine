#pragma once
#ifndef MVULKANSHADER_H
#define MVULKANSHADER_H

#include "Shaders/Shader.hpp"
#include "MVulkanDescriptor.hpp"
#include "vulkan/vulkan_core.h"
#include <spirv_glsl.hpp>
#include <vector>
#include <Managers/singleton.hpp>

class MVulkanShader {
public:
	MVulkanShader(std::string entryPoint = "main", bool compileEveryTime = false);
	MVulkanShader(std::string path, ShaderStageFlagBits stage, std::string entryPoint = "main", bool compileEveryTime = false);

	void Init(std::string path, ShaderStageFlagBits stage);

	void Create(VkDevice device);
	void Clean();
	inline VkShaderModule GetShaderModule() const { return m_shaderModule; }
	inline Shader GetShader() { return m_shader; }

private:
	VkShaderModule createShaderModule(const std::vector<char>& code);

	std::string			m_entryName = "main";
	bool				m_compileEveryTime;
	VkDevice			m_device;
	Shader				m_shader;
	VkShaderModule		m_shaderModule;
};

struct ShaderResourceInfo {
	std::string name;
	ShaderStageFlagBits stage;
	uint32_t set;
	uint32_t binding;
	size_t size;
	uint32_t offset;

	uint32_t descriptorCount;
};

struct ShaderReflectorOut {
	std::vector<ShaderResourceInfo> uniformBuffers;
	std::vector<ShaderResourceInfo> storageBuffers;

	std::vector<ShaderResourceInfo> combinedImageSamplers;
	std::vector<ShaderResourceInfo> seperateSamplers;
	std::vector<ShaderResourceInfo> seperateImages;
	std::vector<ShaderResourceInfo> storageImages;

	std::vector<ShaderResourceInfo> accelarationStructs;

	std::vector<MVulkanDescriptorSetLayoutBinding> GetBindings();
};

class MVulkanShaderReflector {
public:
	MVulkanShaderReflector(Shader shader);

	PipelineVertexInputStateInfo GenerateVertexInputAttributes();
	MVulkanDescriptorSet GenerateDescriptorSet();
	ShaderReflectorOut GenerateShaderReflactorOut();

	void GenerateVertexInputBindingDescription();
private:
	Shader m_shader;

	std::vector<uint32_t>			m_spirvBinary;
	spirv_cross::CompilerGLSL		m_compiler;
	spirv_cross::ShaderResources	m_resources;
};

#endif