#pragma once
#ifndef MVULKANSHADER_H
#define MVULKANSHADER_H

#include "Shader.h"
#include "vulkan/vulkan_core.h"
#include <vector>


class MVulkanShader {
public:
	MVulkanShader();
	MVulkanShader(std::string path, ShaderStageFlagBits stage);

	void Init(std::string path, ShaderStageFlagBits stage);

	void Create(VkDevice device);
	void Clean(VkDevice device);
	inline VkShaderModule GetShaderModule() const { return shaderModule; }


private:
	VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

	Shader shader;

	VkShaderModule shaderModule;
};


#endif