#include "MVulkanCommand.h"

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
	vkQueueWaitIdle(queue);
}
