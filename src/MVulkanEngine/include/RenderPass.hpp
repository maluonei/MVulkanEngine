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
	VkFormat depthFormat;
	std::vector<VkFormat> imageAttachmentFormats;
	VkImageView* colorAttachmentResolvedViews = nullptr;

	//bool generateDepthMipmap = false;
	bool useDepthBuffer = true;
	bool reuseDepthView = false;
	VkImageView depthView = nullptr;

	VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkImageLayout initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentLoadOp  loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	VkAttachmentLoadOp  depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	VkAttachmentLoadOp  stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	MVulkanPilelineCreateInfo pipelineCreateInfo;
};

class RenderPass {
public:
	RenderPass(MVulkanDevice device, RenderPassCreateInfo info);
	void Create(std::shared_ptr<ShaderModule> shader,
		MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
		MVulkanDescriptorSetAllocator& allocator, 
		std::vector<std::vector<VkImageView>> imageViews, 
		std::vector<VkSampler> samplers, 
		std::vector<VkAccelerationStructureKHR> accelerationStructure = {});

	void Create(std::shared_ptr<ShaderModule> shader,
		MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
		MVulkanDescriptorSetAllocator& allocator, 
		std::vector<uint32_t> storageBufferSizes,
		std::vector<std::vector<VkImageView>> imageViews, 
		std::vector<std::vector<VkImageView>> storageImageViews,
		std::vector<VkSampler> samplers,
		std::vector<VkAccelerationStructureKHR> accelerationStructure = {});

	void Create(std::shared_ptr<ShaderModule> shader,
		MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
		MVulkanDescriptorSetAllocator& allocator, 
		std::vector<StorageBuffer> storageBuffers,
		std::vector<std::vector<VkImageView>> imageViews,
		std::vector<std::vector<VkImageView>> storageImageViews,
		std::vector<VkSampler> samplers,
		std::vector<VkAccelerationStructureKHR> accelerationStructure = {});

	void Clean();

	void RecreateFrameBuffers(MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList, VkExtent2D extent);
	
	void UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> imageViews, 
		std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructure = {});
	void UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> imageViews, std::vector<std::vector<VkImageView>> storageImageViews,
		std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructure = {});

	void SetUBO(uint8_t binding, void* data);
	void LoadCBV(uint32_t alignment);
	void LoadStorageBuffer(uint32_t alignment);
	void UpdateCBVData();

	MVulkanRenderPass GetRenderPass() const { return m_renderPass; }
	MVulkanPipeline GetPipeline() const { return m_pipeline; }
	MVulkanFrameBuffer GetFrameBuffer(uint32_t index) const { return m_frameBuffers[index]; }
	MVulkanDescriptorSetLayouts GetDescriptorSetLayouts() const { return m_descriptorLayouts; }
	MVulkanDescriptorSet GetDescriptorSet(int index) const { return m_descriptorSets[index]; }
	
	std::shared_ptr<ShaderModule> GetShader() const { return m_shader; }

	MVulkanDescriptorSet CreateDescriptorSet(MVulkanDescriptorSetAllocator& allocator);

	void TransitionFrameBufferImageLayout(MVulkanCommandQueue& queue, MGraphicsCommandList& commandList,VkImageLayout oldLayout, VkImageLayout newLayout);

	inline uint32_t GetFramebufferCount() { return m_info.frambufferCount; }
	inline uint32_t GetAttachmentCount() { return m_frameBuffers[0].ColorAttachmentsCount(); }

	inline RenderPassCreateInfo& GetRenderPassCreateInfo() { return m_info; }
private:
	void CreatePipeline(MVulkanDescriptorSetAllocator& allocator, 
		std::vector<std::vector<VkImageView>> imageViews, 
		std::vector<VkSampler> samplers, 
		std::vector<VkAccelerationStructureKHR> accelerationStructure);

	void CreatePipeline(MVulkanDescriptorSetAllocator& allocator,
		std::vector<uint32_t> storageBufferSizes,
		std::vector<std::vector<VkImageView>> imageViews, 
		std::vector<std::vector<VkImageView>> storageImageViews, 
		std::vector<VkSampler> samplers,
		std::vector<VkAccelerationStructureKHR> accelerationStructure);

	void CreatePipeline(MVulkanDescriptorSetAllocator& allocator,
		std::vector<StorageBuffer> storageBuffers,
		std::vector<std::vector<VkImageView>> imageViews,
		std::vector<std::vector<VkImageView>> storageImageViews,
		std::vector<VkSampler> samplers,
		std::vector<VkAccelerationStructureKHR> accelerationStructure);

	void CreateRenderPass();
	void CreateFrameBuffers(MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList);
	void SetShader(std::shared_ptr<ShaderModule> shader);

private:
	MVulkanDevice			m_device;
	RenderPassCreateInfo	m_info;

	std::shared_ptr<ShaderModule>	m_shader;
	std::vector<std::vector<MCBV>>	m_uniformBuffers;
	std::vector<StorageBuffer>		m_storageBuffer;

	std::vector<VkDescriptorType>	m_samplerTypes;

	MVulkanRenderPass				m_renderPass;
	MVulkanGraphicsPipeline			m_pipeline;
	std::vector<MVulkanFrameBuffer> m_frameBuffers;

	MVulkanDescriptorSetLayouts		m_descriptorLayouts;
	std::vector<MVulkanDescriptorSet> m_descriptorSets;

	uint32_t m_cbvCount = 0;
	uint32_t m_storageBufferCount = 0;
	uint32_t m_separateSamplerCount = 0;
	uint32_t m_separateImageCount = 0;
	uint32_t m_storageImageCount = 0;
	uint32_t m_combinedImageCount = 0;

	uint32_t m_asCount = 0;
};


#endif