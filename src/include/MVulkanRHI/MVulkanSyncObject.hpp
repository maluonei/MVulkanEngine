#pragma once
#ifndef MVULKANSYNCOBJECT_H
#define MVULKANSYNCOBJECT_H

#include <vulkan/vulkan_core.h>

class MVulkanFence {
public:

	void Create(VkDevice device);
	void Clean(VkDevice device);
	void WaitForSignal(VkDevice device);
	void Reset(VkDevice device);
	inline VkFence GetFence() const {return fence;}

private:
	VkFence fence;
};

class MVulkanSemaphore {
public:

	void Create(VkDevice device);
	void Clean(VkDevice device);
	inline VkSemaphore GetSemaphore() const {return semaphore;}

private:
	VkSemaphore semaphore;
};

#endif