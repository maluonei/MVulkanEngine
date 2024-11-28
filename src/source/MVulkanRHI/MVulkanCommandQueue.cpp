#include "MVulkanRHI/MVulkanCommand.hpp"

MVulkanCommandQueue::MVulkanCommandQueue()
{
}

void MVulkanCommandQueue::SetQueue(VkQueue _queue)
{
	queue = _queue;
}

void MVulkanCommandQueue::SubmitCommands(uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
	VK_CHECK_RESULT(vkQueueSubmit(queue, submitCount, pSubmits, fence));
}


void MVulkanCommandQueue::WaitForQueueComplete()
{
	VK_CHECK_RESULT(vkQueueWaitIdle(queue));
}

VkResult MVulkanCommandQueue::Present(VkPresentInfoKHR* presentInfo)
{
	return vkQueuePresentKHR(queue, presentInfo);
}
