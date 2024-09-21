#pragma once
#ifndef MVULKANCOMMAND_H
#define MVULKANCOMMAND_H

#include "vulkan/vulkan_core.h"
#include "MVulkanDevice.h"
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
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void Create(VkDevice device, const VkCommandListCreateInfo& info);
	void Clean(VkDevice _device);

	inline VkCommandBuffer& GetBuffer() {
		return commandBuffer;
	};

private:
	VkCommandPool		 commandPool;
	VkCommandBuffer      commandBuffer;
	VkCommandBufferLevel level;
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

	inline VkQueue Get() const { return queue; }

private:
	VkQueue queue;
};



#endif