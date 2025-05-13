#include "MVulkanRHI/MVulkanQueryPool.hpp"

void MVulkanQueryPool::Create(MVulkanDevice device, VkQueryType queryType)
{
	m_device = device.GetDevice();

	VkQueryPoolCreateInfo queryPoolInfo{};
	queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolInfo.queryType = queryType;
	queryPoolInfo.queryCount = 1024;  // һ����ʼ��һ������

	VK_CHECK_RESULT(vkCreateQueryPool(m_device, &queryPoolInfo, nullptr, &m_queryPool));
}

VkQueryPool MVulkanQueryPool::Get()
{
	return m_queryPool;
}
