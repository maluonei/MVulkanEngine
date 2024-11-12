#pragma once
#ifndef RENDERPASS_H
#define RENDERPASS_H

#include "MVulkanRHI/MVulkanPipeline.hpp"
#include "MVulkanRHI/MVulkanRenderPass.hpp"
#include "MVulkanRHI/MVulkanFrameBuffer.hpp"
#include "MVulkanRHI/MVulkanDevice.hpp"
#include "MVulkanRHI/MVulkanDescriptor.hpp"
#include "MVulkanRHI/MVulkanCommand.hpp"
#include "MVulkanRHI/MVulkanSwapchain.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanSampler.hpp"

class ShaderModule;

struct RenderPassCreateInfo {
	uint32_t frambufferCount = 1;
	bool useAttachmentResolve = false;
	bool useSwapchainImages = false;
	VkExtent2D extent;
	std::vector<VkFormat> imageAttachmentFormats;
	VkImageView* colorAttachmentResolvedViews = nullptr;

	VkImageLayout initialLayout;
	VkImageLayout finalLayout;
};

class RenderPass {
public:
	RenderPass(MVulkanDevice device, RenderPassCreateInfo info);
	void Create(std::shared_ptr<ShaderModule> shader,
		MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList,
		MVulkanDescriptorSetAllocator allocator, std::vector<std::vector<VkImageView>> imageViews);

	void Clean();

	void RecreateFrameBuffers(MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList, VkExtent2D extent);
	void UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> imageViews);

	void SetUBO(uint8_t index, void* data);
	void LoadCBV();

	MVulkanRenderPass GetRenderPass() const { return m_renderPass; }
	MVulkanPipeline GetPipeline() const { return m_pipeline; }
	MVulkanFrameBuffer GetFrameBuffer(uint32_t index) const { return m_frameBuffers[index]; }
	MVulkanDescriptorSetLayouts GetDescriptorSetLayouts() const { return m_descriptorLayouts; }
	MVulkanDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }
	std::shared_ptr<ShaderModule> GetShader() const { return m_shader; }

	void TransitionFrameBufferImageLayout(MVulkanCommandQueue queue, MGraphicsCommandList commandList,VkImageLayout oldLayout, VkImageLayout newLayout);

private:
	void CreatePipeline(MVulkanDescriptorSetAllocator allocator, std::vector<std::vector<VkImageView>> imageViews);
	void CreateRenderPass();
	void CreateFrameBuffers(MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList);
	void SetShader(std::shared_ptr<ShaderModule> shader);
	//void CreateShaderResources();

private:
	MVulkanDevice m_device;
	RenderPassCreateInfo m_info;

	std::shared_ptr<ShaderModule> m_shader;
	std::unordered_map<uint32_t, std::vector<MCBV>> m_uniformBuffers;
	std::unordered_map<uint32_t, std::vector<MVulkanSampler>> m_samplers;

	MVulkanRenderPass m_renderPass;
	MVulkanGraphicsPipeline m_pipeline;
	std::vector<MVulkanFrameBuffer> m_frameBuffers;

	MVulkanDescriptorSetLayouts m_descriptorLayouts;
	MVulkanDescriptorSet m_descriptorSet;

	uint32_t m_cbvCount = 0, m_textureCount = 0;
};


#endif