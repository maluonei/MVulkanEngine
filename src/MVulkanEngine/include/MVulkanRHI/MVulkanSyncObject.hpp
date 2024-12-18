#pragma once
#ifndef MVULKANSYNCOBJECT_H
#define MVULKANSYNCOBJECT_H

#include <vulkan/vulkan_core.h>

class MVulkanFence {
public:
	MVulkanFence() = default;
	~MVulkanFence();
	void Create(VkDevice device);
	void Clean();
	void WaitForSignal();
	void Reset();
	inline VkFence GetFence() const {return m_fence;}

private:
	VkDevice	m_device;
	VkFence		m_fence;
};

class MVulkanSemaphore {
public:

	void Create(VkDevice device);
	void Clean();
	inline VkSemaphore GetSemaphore() const {return m_semaphore;}

private:
	VkDevice	m_device;
	VkSemaphore m_semaphore;
};

#endif