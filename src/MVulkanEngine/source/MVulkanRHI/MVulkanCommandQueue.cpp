#include "MVulkanRHI/MVulkanCommand.hpp"

MVulkanCommandQueue::MVulkanCommandQueue()
{
}

void MVulkanCommandQueue::SetQueue(VkQueue _queue)
{
	m_queue = _queue;
}

void MVulkanCommandQueue::Clean()
{
	vkQueueWaitIdle(m_queue);
}

void MVulkanCommandQueue::SubmitCommands(uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
	VK_CHECK_RESULT(vkQueueSubmit(m_queue, submitCount, pSubmits, fence));
}


void MVulkanCommandQueue::WaitForQueueComplete()
{
	VK_CHECK_RESULT(vkQueueWaitIdle(m_queue));
}

VkResult MVulkanCommandQueue::Present(VkPresentInfoKHR* presentInfo)
{
	return vkQueuePresentKHR(m_queue, presentInfo);
}
