#pragma once
#ifndef MVULKANSHADER_H
#define MVULKANSHADER_H

#include "Shader.h"
#include "vulkan/vulkan_core.h"
#include <spirv_glsl.hpp>
#include <vector>
#include <singleton.h>


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

class MVulkanShaderReflector {
public:
	MVulkanShaderReflector(Shader shader);

	//void loadShader(Shader shader);
	PipelineVertexInputStateInfo GenerateVertexInputAttributes();
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