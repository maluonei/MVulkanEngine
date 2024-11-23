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
	MVulkanShader();
	MVulkanShader(std::string path, ShaderStageFlagBits stage);

	void Init(std::string path, ShaderStageFlagBits stage);

	void Create(VkDevice device);
	void Clean(VkDevice device);
	inline VkShaderModule GetShaderModule() const { return shaderModule; }
	inline Shader GetShader() { return shader; }

private:
	VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

	Shader shader;
	VkShaderModule shaderModule;
};

struct ShaderResourceInfo {
	std::string name;
	ShaderStageFlagBits stage;
	uint32_t set;
	uint32_t binding;
	size_t size;
	uint32_t offset;

	uint32_t descriptorCount;
	//ShaderResourceInfo(const std::string& _name, uint32_t _set, )
};

struct ShaderReflectorOut {
	std::vector<ShaderResourceInfo> uniformBuffers;
	std::vector<ShaderResourceInfo> storageBuffers;
	std::vector<ShaderResourceInfo> samplers;

	std::vector<MVulkanDescriptorSetLayoutBinding> GetUniformBufferBindings();
	std::vector<MVulkanDescriptorSetLayoutBinding> GetSamplers();
	std::vector<MVulkanDescriptorSetLayoutBinding> GetBindings();

};

class MVulkanShaderReflector {
public:
	MVulkanShaderReflector(Shader shader);

	//void loadShader(Shader shader);
	PipelineVertexInputStateInfo GenerateVertexInputAttributes();
	MVulkanDescriptorSet GenerateDescriptorSet();
	ShaderReflectorOut GenerateShaderReflactorOut();

	void GenerateVertexInputBindingDescription();

	void test(Shader shader);

private:
	Shader shader;

	std::vector<uint32_t> spirvBinary;
	// ´´½¨ SPIRV-Cross µÄ GLSL ±àÒëÆ÷
	spirv_cross::CompilerGLSL compiler;
	spirv_cross::ShaderResources resources;
};

#endif