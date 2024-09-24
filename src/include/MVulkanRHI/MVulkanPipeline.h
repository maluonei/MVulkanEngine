#pragma once
#ifndef MVULKANPIPELINE_H
#define MVULKANPIPELINE_H

#include "vulkan/vulkan_core.h"
#include "Utils/VulkanUtil.h"
#include <vector>

class MVulkanPipeline {
public:
	MVulkanPipeline();

	void Create();
	void Clean(VkDevice device);
	inline VkPipeline Get() const { return pipeline; }
protected:
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};


class MVulkanGraphicsPipeline : public MVulkanPipeline {
public:
	
	void Create(VkDevice device, VkShaderModule vertShaderModule, VkShaderModule fragShaderModule, VkRenderPass renderPass,
		PipelineVertexInputStateInfo vertexStateInfo);

private:



};
#endif