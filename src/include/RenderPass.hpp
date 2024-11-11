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
	bool useAttachmentResolve = false;
	bool useSwapchainImages = false;
	VkExtent2D extent;
	std::vector<VkFormat> imageAttachmentFormats;
	VkImageView* colorAttachmentResolvedViews = nullptr;
};

class RenderPass {
public:
	RenderPass(MVulkanDevice device, RenderPassCreateInfo info);
	void Create(std::shared_ptr<ShaderModule> shader,
		MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList,
		MVulkanDescriptorSetAllocator allocator, std::vector<VkImageView> imageViews);

	void Clean();

	void RecreateFrameBuffers(MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList, VkExtent2D extent);

	void SetUBO(uint8_t index, void* data);

	VkRenderPass GetRenderPass() const { return m_renderPass.Get(); }
	VkPipeline GetPipeline() const { return m_pipeline.Get(); }
	VkFramebuffer GetFrameBuffer(uint32_t index) const { return m_frameBuffers[index].Get(); }

private:
	void CreatePipeline(MVulkanDescriptorSetAllocator allocator, std::vector<VkImageView> imageViews);
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
};


#endif