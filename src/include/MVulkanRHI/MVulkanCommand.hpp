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

	void TransitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout);
	void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, unsigned int width, unsigned int height);
	void CopyImage(VkImage srcImage, VkImage dstImage, unsigned int width, unsigned int height);

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