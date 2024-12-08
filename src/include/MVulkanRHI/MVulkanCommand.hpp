#pragma once
#ifndef MVULKANCOMMAND_H
#define MVULKANCOMMAND_H

#include "vulkan/vulkan_core.h"
#include "MVulkanDevice.hpp"
#include "map"

struct VkCommandListCreateInfo {
	VkCommandPool commandPool;
	VkCommandBufferLevel level;
	uint32_t commandBufferCount;
};

class MVulkanCommandAllocator {
public:
	MVulkanCommandAllocator();
	void Create(MVulkanDevice device);
	void Clean(VkDevice device);

	inline VkCommandPool Get(QueueType type) {
		return commandPools[type];
	}

private:
	//VkCommandPool commandPool;
	std::map<QueueType, VkCommandPool> commandPools;
};

struct MVulkanImageCopyInfo {
	VkImageAspectFlags srcAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageAspectFlags dstAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	uint32_t srcMpiLevel = 0;
	uint32_t dstMpiLevel = 0;
};

struct MVulkanImageMemoryBarrier {
	VkAccessFlags              srcAccessMask = 0;
	VkAccessFlags              dstAccessMask = 0;
	VkImageLayout              oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout              newLayout = VK_IMAGE_LAYOUT_GENERAL;
	uint32_t                   srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32_t                   dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	VkImage                    image;
	VkImageAspectFlags		   aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	uint32_t				   baseMipLevel = 0;
	uint32_t				   levelCount = 1;
	uint32_t				   baseArrayLayer = 0;
	uint32_t				   layerCount = 1;
};

class MVulkanCommandList {
public:
	MVulkanCommandList();
	MVulkanCommandList(VkDevice device, const VkCommandListCreateInfo& info);

	void Begin();
	void End();
	void Reset();
	void BeginRenderPass(VkRenderPassBeginInfo* info);
	void EndRenderPass();
	virtual void BindPipeline(VkPipeline pipeline) = 0;
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void TransitionImageLayout(std::vector<MVulkanImageMemoryBarrier> _barrier, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
	void TransitionImageLayout(MVulkanImageMemoryBarrier _barrier, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
	//void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, unsigned int width, unsigned int height, uint32_t mipLevel=0, uint32_t baseArrayLayer=0);
	void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, unsigned int width, unsigned int height, uint32_t bufferOffset = 0, uint32_t mipLevel = 0, uint32_t baseArrayLayer = 0);
	void CopyImage(VkImage srcImage, VkImage dstImage, unsigned int width, unsigned int height, MVulkanImageCopyInfo copyInfo);

	void BlitImage(
		VkImage srcImage, VkImageLayout srcLayout,
		VkImage dstImage, VkImageLayout dstLayout, 
		std::vector<VkImageBlit> blits, VkFilter filter);

	void Create(VkDevice device, const VkCommandListCreateInfo& info);
	void Clean(VkDevice _device);

	inline VkCommandBuffer& GetBuffer() {
		return commandBuffer;
	};

protected:
	//void endSingleTimeCommands(VkDevice device);

	VkCommandPool		 commandPool;
	VkCommandBuffer      commandBuffer;
	VkCommandBufferLevel level;
};

class MGraphicsCommandList :public MVulkanCommandList {
public:
	void BindPipeline(VkPipeline pipeline);
	void SetViewport(uint32_t firstViewport, uint32_t viewportNum, VkViewport* viewport);
	void SetScissor(uint32_t firstScissor, uint32_t scissorNum, VkRect2D* scissor);
	void BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffer, const VkDeviceSize* offset);
	void BindIndexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer buffer, const VkDeviceSize* offset);
	void BindDescriptorSet(VkPipelineLayout pipelineLayout, uint32_t firstSet, uint32_t descriptorSetCount, VkDescriptorSet* set);
	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
	void DrawIndexedIndirectCommand(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
};

class MComputeCommandList :public MVulkanCommandList {
public:
	void BindPipeline(VkPipeline pipeline);
};

class MVulkanCommandQueue {
public:
	MVulkanCommandQueue();
	void SetQueue(VkQueue queue);
	
	void SubmitCommands(
		uint32_t submitCount,
		const VkSubmitInfo* pSubmits,
		VkFence fence);

	void WaitForQueueComplete();

	VkResult Present(VkPresentInfoKHR* presentInfo);

	inline VkQueue Get() const { return queue; }

private:
	VkQueue queue;
};



#endif