#ifndef MVULKAN_QUERY_POOL_HPP
#define MVULKAN_QUERY_POOL_HPP

#include "MVulkanDevice.hpp"

class MVulkanQueryPool {
public:
	void Create(MVulkanDevice device, VkQueryType queryType);
	VkQueryPool Get();

	void Reset();
	void Reset(uint32_t        firstQuery,
				uint32_t        queryCount);
	std::vector<uint64_t> GetQueryResults(int numQuerys);
	std::vector<uint64_t> GetQueryResults(int startQuery, int numQuerys);
private:
	VkDevice	m_device;
	VkQueryPool m_queryPool;
};

#endif