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
#include <variant>
#include "MVulkanRHI/MVulkanDescriptorWrite.hpp"

class ShaderModule;
class MeshShaderModule;

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
	bool dynamicRender = false;
};

struct ShaderResourceKey {
	int set;
	int binding;
	bool operator<(const ShaderResourceKey& other) const {
		if (this->set == other.set) {
			return this->binding < other.binding;
		}
		return this->set < other.set;
	}

	bool operator==(const ShaderResourceKey& other) const {
		return set == other.set && binding == other.binding;
	}
};

namespace std {
	template <>
	struct hash<ShaderResourceKey> {
		std::size_t operator()(const ShaderResourceKey& k) const {
			return std::hash<int>()(k.set) ^ (std::hash<int>()(k.binding) << 1);
		}
	};
}

class RenderPass {
public:
	RenderPass(MVulkanDevice device, RenderPassCreateInfo info);
	//void Create(std::shared_ptr<ShaderModule> shader,
	//	MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
	//	MVulkanDescriptorSetAllocator& allocator, 
	//	std::vector<std::vector<VkImageView>> imageViews, 
	//	std::vector<VkSampler> samplers, 
	//	std::vector<VkAccelerationStructureKHR> accelerationStructure = {});
	//
	//void Create(std::shared_ptr<ShaderModule> shader,
	//	MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
	//	MVulkanDescriptorSetAllocator& allocator, 
	//	std::vector<uint32_t> storageBufferSizes,
	//	std::vector<std::vector<VkImageView>> imageViews, 
	//	std::vector<std::vector<VkImageView>> storageImageViews,
	//	std::vector<VkSampler> samplers,
	//	std::vector<VkAccelerationStructureKHR> accelerationStructure = {});
	//
	//void Create(std::shared_ptr<ShaderModule> shader,
	//	MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
	//	MVulkanDescriptorSetAllocator& allocator, 
	//	std::vector<StorageBuffer> storageBuffers,
	//	std::vector<std::vector<VkImageView>> imageViews,
	//	std::vector<std::vector<VkImageView>> storageImageViews,
	//	std::vector<VkSampler> samplers,
	//	std::vector<VkAccelerationStructureKHR> accelerationStructure = {});
	//
	//void Create(
	//	std::shared_ptr<MeshShaderModule> shader,
	//	MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
	//	MVulkanDescriptorSetAllocator& allocator,
	//	std::vector<StorageBuffer> storageBuffers,
	//	std::vector<std::vector<VkImageView>> imageViews,
	//	std::vector<std::vector<VkImageView>> storageImageViews,
	//	std::vector<VkSampler> samplers,
	//	std::vector<VkAccelerationStructureKHR> accelerationStructure = {});

	//void Create(
	//	std::shared_ptr<ShaderModule> shader,
	//	MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
	//	MVulkanDescriptorSetAllocator& allocator,
	//	std::vector<PassResources> resources
	//	);

	void Create(
		std::shared_ptr<ShaderModule> shader,
		MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
		MVulkanDescriptorSetAllocator& allocator
	);

	void CreateDynamic(
		std::shared_ptr<ShaderModule> shader,
		MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
		MVulkanDescriptorSetAllocator& allocator
	);

	void Clean();

	void RecreateFrameBuffers(MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList, VkExtent2D extent);
	
	//void UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> imageViews, 
	//	std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructure = {});
	//void UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> imageViews, std::vector<std::vector<VkImageView>> storageImageViews,
	//	std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructure = {});

	void UpdateDescriptorSetWrite(int frameIndex, std::vector<PassResources> resources);

	void SetUBO(uint8_t binding, void* data);
	void LoadCBV(uint32_t alignment);
	void LoadStorageBuffer(uint32_t alignment);
	void UpdateCBVData();

	MVulkanRenderPass GetRenderPass() const { return m_renderPass; }
	MVulkanPipeline GetPipeline() const { return m_pipeline; }
	MVulkanFrameBuffer GetFrameBuffer(uint32_t index) const { return m_frameBuffers[index]; }
	MVulkanDescriptorSetLayouts GetDescriptorSetLayouts(int frameIndex) const { return m_descriptorLayouts[frameIndex]; }
	//MVulkanDescriptorSet GetDescriptorSet(int index) const { return m_descriptorSets[index]; }
	//std::vector<MVulkanDescriptorSet> GetDescriptorSets(int frameIndex) const { return m_descriptorSets[index]; }
	std::unordered_map<int, MVulkanDescriptorSet> GetDescriptorSets(int frameIndex) const { return m_descriptorSets[frameIndex]; }

	std::shared_ptr<ShaderModule> GetShader() const { return m_shader; }

	std::shared_ptr<MeshShaderModule> GetMeshShader() const { return m_meshShader; }

	//MVulkanDescriptorSet CreateDescriptorSet(MVulkanDescriptorSetAllocator& allocator);

	void TransitionFrameBufferImageLayout(MVulkanCommandQueue& queue, MGraphicsCommandList& commandList,VkImageLayout oldLayout, VkImageLayout newLayout);

	inline uint32_t GetFramebufferCount() { return m_info.frambufferCount; }
	inline uint32_t GetAttachmentCount() { return m_frameBuffers[0].ColorAttachmentsCount(); }

	inline RenderPassCreateInfo& GetRenderPassCreateInfo() { return m_info; }

	void PrepareResourcesForShaderRead(int currentFrame);
private:
	//void CreatePipeline(MVulkanDescriptorSetAllocator& allocator, 
	//	std::vector<std::vector<VkImageView>> imageViews, 
	//	std::vector<VkSampler> samplers, 
	//	std::vector<VkAccelerationStructureKHR> accelerationStructure);
	//
	//void CreatePipeline(MVulkanDescriptorSetAllocator& allocator,
	//	std::vector<uint32_t> storageBufferSizes,
	//	std::vector<std::vector<VkImageView>> imageViews, 
	//	std::vector<std::vector<VkImageView>> storageImageViews, 
	//	std::vector<VkSampler> samplers,
	//	std::vector<VkAccelerationStructureKHR> accelerationStructure);
	//
	//void CreatePipeline(MVulkanDescriptorSetAllocator& allocator,
	//	std::vector<StorageBuffer> storageBuffers,
	//	std::vector<std::vector<VkImageView>> imageViews,
	//	std::vector<std::vector<VkImageView>> storageImageViews,
	//	std::vector<VkSampler> samplers,
	//	std::vector<VkAccelerationStructureKHR> accelerationStructure);

	//void CreateMeshShaderPipeline(MVulkanDescriptorSetAllocator& allocator,
	//	std::vector<StorageBuffer> storageBuffers,
	//	std::vector<std::vector<VkImageView>> imageViews,
	//	std::vector<std::vector<VkImageView>> storageImageViews,
	//	std::vector<VkSampler> samplers,
	//	std::vector<VkAccelerationStructureKHR> accelerationStructure);
	//
	//void CreatePipeline(
	//	MVulkanDescriptorSetAllocator& allocator,
	//	std::vector<PassResources> resources);

	void CreatePipeline(
		MVulkanDescriptorSetAllocator& allocator);

	void CreateRenderPass();
	void CreateFrameBuffers(MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList);
	void SetShader(std::shared_ptr<ShaderModule> shader);
	void SetMeshShader(std::shared_ptr<MeshShaderModule> meshShader);

	
	//void PrepareResourcesForPresent();
private:
	MVulkanDevice			m_device;
	RenderPassCreateInfo	m_info;

	//union Shader{
	std::shared_ptr<ShaderModule>	m_shader = nullptr;
	std::shared_ptr<MeshShaderModule> m_meshShader = nullptr;
	//};
		//std::shared_ptr<ShaderModule>	m_shader;
	std::vector<std::vector<MCBV>>	m_uniformBuffers;
	std::vector<StorageBuffer>		m_storageBuffer;

	std::vector<VkDescriptorType>	m_samplerTypes;

	MVulkanRenderPass				m_renderPass;
	MVulkanGraphicsPipeline			m_pipeline;
	std::vector<MVulkanFrameBuffer> m_frameBuffers;

	//MVulkanDescriptorSetLayouts		m_descriptorLayouts;
	std::vector<MVulkanDescriptorSetLayouts> m_descriptorLayouts;

	//std::vector<>
	std::vector<std::unordered_map<int, MVulkanDescriptorSet>> m_descriptorSets;

	uint32_t m_cbvCount = 0;
	uint32_t m_storageBufferCount = 0;
	uint32_t m_separateSamplerCount = 0;
	uint32_t m_separateImageCount = 0;
	uint32_t m_storageImageCount = 0;
	uint32_t m_combinedImageCount = 0;

	uint32_t m_asCount = 0;

	void updateResourceCache(ShaderResourceKey key, const PassResources& resources);
	//void mergeBindings(
	//std::unordered_map<ShaderResourceKey, PassResources> m_cachedResources;
	//std::vector<std::vector<MVulkanDescriptorSetLayoutBinding>> m_bindings;
	std::unordered_map<ShaderResourceKey, MVulkanDescriptorSetLayoutBinding> m_bindings;
	std::unordered_map<ShaderResourceKey, PassResources> m_cachedResources;
	//std::vector<>
};

//namespace std {
//	template <>
//	struct hash<ShaderResourceKey> {
//		std::size_t operator()(const ShaderResourceKey& k) const {
//			return std::hash<int>()(k.set) ^ (std::hash<int>()(k.binding) << 1);
//		}
//	};
//}
//
struct DynamicRenderPassCreateInfo {
	MVulkanPilelineCreateInfo pipelineCreateInfo;
	//std::vector<VkFormat> colorAttachmentFormats;
	//VkFormat depthAttachmentFormats;
	//VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	uint8_t numFrameBuffers = 1;
	//uint8_t numAttachments = 1;
};

class DynamicRenderPass {
public:
	DynamicRenderPass(MVulkanDevice device, DynamicRenderPassCreateInfo info);

	void Create(
		std::shared_ptr<ShaderModule> shader,
		MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
		MVulkanDescriptorSetAllocator& allocator,
		std::vector<StorageBuffer> storageBuffers,
		std::vector<std::vector<VkImageView>> imageViews,
		std::vector<std::vector<VkImageView>> storageImageViews,
		std::vector<VkSampler> samplers,
		std::vector<VkAccelerationStructureKHR> accelerationStructure = {});

	MVulkanPipeline GetPipeline() const { return m_pipeline; }
	MVulkanDescriptorSetLayouts GetDescriptorSetLayouts() const { return m_descriptorLayouts; }
	MVulkanDescriptorSet GetDescriptorSet(int index) const { return m_descriptorSets[index]; }

	std::shared_ptr<ShaderModule> GetShader() const { return m_shader; }

	std::shared_ptr<MeshShaderModule> GetMeshShader() const { return m_meshShader; }

	MVulkanDescriptorSet CreateDescriptorSet(MVulkanDescriptorSetAllocator& allocator);
	
	void UpdateDescriptorSetWrite(
		std::vector<StorageBuffer> storageBuffers,
		std::vector<std::vector<VkImageView>> imageViews, 
		std::vector<std::vector<VkImageView>> storageImageViews,
		std::vector<VkSampler> samplers, 
		std::vector<VkAccelerationStructureKHR> accelerationStructure = {});

	void TransitionFrameBufferImageLayout(MVulkanCommandQueue& queue, MGraphicsCommandList& commandList, VkImageLayout oldLayout, VkImageLayout newLayout);

	void LoadCBV(uint32_t alignment);
private:
	void CreatePipeline(
		MVulkanDescriptorSetAllocator& allocator,
		std::vector<StorageBuffer> storageBuffers,
		std::vector<std::vector<VkImageView>> imageViews,
		std::vector<std::vector<VkImageView>> storageImageViews,
		std::vector<VkSampler> samplers,
		std::vector<VkAccelerationStructureKHR> accelerationStructure);

	void SetShader(std::shared_ptr<ShaderModule> shader);
	void SetMeshShader(std::shared_ptr<MeshShaderModule> meshShader);

private:
	MVulkanDevice			m_device;
	DynamicRenderPassCreateInfo	m_info;

	MVulkanGraphicsPipeline			m_pipeline;

	MVulkanDescriptorSetLayouts		m_descriptorLayouts;
	std::vector<MVulkanDescriptorSet> m_descriptorSets;

	std::shared_ptr<ShaderModule>	m_shader = nullptr;
	std::shared_ptr<MeshShaderModule> m_meshShader = nullptr;

	std::vector<std::vector<MCBV>>	m_uniformBuffers;

	uint32_t m_cbvCount = 0;
	uint32_t m_storageBufferCount = 0;
	uint32_t m_separateSamplerCount = 0;
	uint32_t m_separateImageCount = 0;
	uint32_t m_storageImageCount = 0;
	uint32_t m_combinedImageCount = 0;

	uint32_t m_asCount = 0;




};


#endif