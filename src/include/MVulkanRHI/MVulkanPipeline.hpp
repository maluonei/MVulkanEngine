#pragma once
#ifndef MVULKANPIPELINE_H
#define MVULKANPIPELINE_H

#include "vulkan/vulkan_core.h"
#include "Utils/VulkanUtil.hpp"
#include <vector>
#include "MVulkanDescriptor.hpp"

class MVulkanPipeline {
public:
	MVulkanPipeline();

	void Create();
	void Clean(VkDevice device);
	inline VkPipeline Get() const { return pipeline; }
	inline VkPipelineLayout GetLayout() const { return pipelineLayout; }
protected:
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};


class MVulkanGraphicsPipeline : public MVulkanPipeline {
public:
	
	void Create(VkDevice device, VkShaderModule vertShaderModule, VkShaderModule fragShaderModule, VkRenderPass renderPass,
		PipelineVertexInputStateInfo vertexStateInfo, VkDescriptorSetLayout layout);

private:



};
#endif