#pragma once
#ifndef MVULKANCOMMAND_H
#define MVULKANCOMMAND_H

#include "vulkan/vulkan_core.h"
#include "MVulkanDevice.h"
#include "map"

class MVulkanCommandAllocator {
public:
	MVulkanCommandAllocator();
	void Create(MVulkanDevice device);
	void Clean(VkDevice device);

private:
	//VkCommandPool commandPool;
	std::map<QueueType, VkCommandPool> commandPools;
};

class MVulkanCommandList {
public:
	MVulkanCommandList();

	void Create(VkDevice _device);
	void Clean(VkDevice _device);

private:
	VkCommandBuffer      commandBuffer;
	VkCommandBufferLevel level;
};

class MVulkanCommandQueue {


};



#endif