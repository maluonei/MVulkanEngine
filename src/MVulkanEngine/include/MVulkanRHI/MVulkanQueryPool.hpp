#ifndef MVULKAN_QUERY_POOL_HPP
#define MVULKAN_QUERY_POOL_HPP

#include "MVulkanDevice.hpp"

class MVulkanQueryPool {
public:
	void Create(MVulkanDevice device, VkQueryType queryType);
	VkQueryPool Get();
private:
	VkDevice	m_device;
	VkQueryPool m_queryPool;
};

#endif