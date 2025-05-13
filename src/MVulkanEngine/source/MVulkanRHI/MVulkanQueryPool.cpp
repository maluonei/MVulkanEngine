#include "MVulkanRHI/MVulkanQueryPool.hpp"

void MVulkanQueryPool::Create(MVulkanDevice device, VkQueryType queryType)
{
	m_device = device.GetDevice();

	VkQueryPoolCreateInfo queryPoolInfo{};
	queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolInfo.queryType = queryType;
	queryPoolInfo.queryCount = 1024;  

	VK_CHECK_RESULT(vkCreateQueryPool(m_device, &queryPoolInfo, nullptr, &m_queryPool));
}

VkQueryPool MVulkanQueryPool::Get()
{
	return m_queryPool;
}

void MVulkanQueryPool::Reset()
{
	vkResetQueryPool(m_device, m_queryPool, 0, 1024);
}

void MVulkanQueryPool::Reset(uint32_t firstQuery, uint32_t queryCount)
{
	vkResetQueryPool(m_device, m_queryPool, firstQuery, queryCount);
}

std::vector<uint64_t> MVulkanQueryPool::GetQueryResults(int numQuerys)
{
	std::vector<uint64_t> results(numQuerys);

	VK_CHECK_RESULT(vkGetQueryPoolResults(
		m_device,
		m_queryPool,
		0, numQuerys,
		numQuerys * sizeof(uint64_t),
		results.data(),
		sizeof(uint64_t),
		VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
	));

	return results;
}

std::vector<uint64_t> MVulkanQueryPool::GetQueryResults(int startQuery, int numQuerys)
{
	std::vector<uint64_t> results(numQuerys);

	VK_CHECK_RESULT(vkGetQueryPoolResults(
		m_device,
		m_queryPool,
		startQuery, numQuerys,
		numQuerys * sizeof(uint64_t),
		results.data(),
		sizeof(uint64_t),
		VK_QUERY_RESULT_64_BIT
	));

	return results;
}
