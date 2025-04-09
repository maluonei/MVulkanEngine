#pragma once
#ifndef MVULKANPIPELINE_H
#define MVULKANPIPELINE_H

#include "vulkan/vulkan_core.h"
#include "Utils/VulkanUtil.hpp"
#include <vector>
#include "MVulkanDescriptor.hpp"
#include "MVulkanDevice.hpp"

struct MVulkanPilelineCreateInfo {
	bool depthTestEnable = VK_TRUE;
	bool depthWriteEnable = VK_TRUE;
	VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

	VkCullModeFlags cullmode = VK_CULL_MODE_BACK_BIT;
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	float lineWidth = 1.f;

	std::vector<VkFormat> colorAttachmentFormats;
	VkFormat depthAttachmentFormats;
	VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
};

class MVulkanPipeline {
public:
	MVulkanPipeline();

	void Create();
	void Clean();
	inline VkPipeline Get() const { return m_pipeline; }
	inline VkPipelineLayout GetLayout() const { return m_pipelineLayout; }
protected:
	VkDevice					m_device;
	VkPipeline					m_pipeline;
	VkPipelineLayout			m_pipelineLayout;
	MVulkanPilelineCreateInfo	m_info;
};


class MVulkanGraphicsPipeline : public MVulkanPipeline {
public:
	void Create(VkDevice device, 
		MVulkanPilelineCreateInfo info, 
		VkShaderModule vertShaderModule, 
		VkShaderModule fragShaderModule, 
		VkRenderPass renderPass,
		PipelineVertexInputStateInfo vertexStateInfo,
		VkDescriptorSetLayout layout, 
		uint32_t numAttachments,
		std::string vertEntryPoint = "main",
		std::string fragEntryPoint = "main");

	void Create(VkDevice device,
		MVulkanPilelineCreateInfo info,
		VkShaderModule vertShaderModule,
		VkShaderModule geomShaderModule,
		VkShaderModule fragShaderModule,
		VkRenderPass renderPass,
		PipelineVertexInputStateInfo vertexStateInfo,
		VkDescriptorSetLayout layout,
		uint32_t numAttachments,
		std::string vertEntryPoint = "main",
		std::string geomEntryPoint = "main",
		std::string fragEntryPoint = "main");

	void Create(VkDevice device,
		MVulkanPilelineCreateInfo info,
		VkShaderModule vertShaderModule,
		VkShaderModule geomShaderModule,
		VkShaderModule fragShaderModule,
		VkRenderPass renderPass,
		PipelineVertexInputStateInfo vertexStateInfo,
		std::vector<VkDescriptorSetLayout> layout,
		uint32_t numAttachments,
		std::string vertEntryPoint = "main",
		std::string geomEntryPoint = "main",
		std::string fragEntryPoint = "main");

	void Create(VkDevice device,
		MVulkanPilelineCreateInfo info,
		VkShaderModule taskShaderModule,
		VkShaderModule meshShaderModule,
		VkShaderModule fragShaderModule,
		VkRenderPass renderPass,
		VkDescriptorSetLayout layout,
		uint32_t numAttachments,
		std::string taskEntryPoint = "main",
		std::string meshEntryPoint = "main",
		std::string fragEntryPoint = "main");

	void Create(VkDevice device,
		MVulkanPilelineCreateInfo info,
		VkShaderModule vertShaderModule,
		VkShaderModule geomShaderModule,
		VkShaderModule fragShaderModule,
		PipelineVertexInputStateInfo vertexStateInfo,
		VkDescriptorSetLayout layout,
		std::string vertEntryPoint = "main",
		std::string geomEntryPoint = "main",
		std::string fragEntryPoint = "main");


private:




};

class MVulkanRaytracingPipeline :public MVulkanPipeline {
public:
	void Create(VkDevice device);

private:
	//std::string m_vertEntryPoint;
	//std::string m_fragEntryPoint;
};

class MVulkanComputePipeline :public MVulkanPipeline {
public:
	void Create(VkDevice device, 
		VkShaderModule compShaderModule, 
		VkDescriptorSetLayout layout,
		std::string entryPoint = "");
private:
	//std::string m_entryPoint;
};

#endif