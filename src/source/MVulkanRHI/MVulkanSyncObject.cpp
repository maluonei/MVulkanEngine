#include "MVulkanRHI/MVulkanSyncObject.hpp"
#include "Utils/VulkanUtil.hpp"

void MVulkanFence::Create(VkDevice device)
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
}

void MVulkanFence::Clean(VkDevice device)
{
    vkDestroyFence(device, fence, nullptr);
}

void MVulkanFence::WaitForSignal(VkDevice device)
{
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
}

void MVulkanFence::Reset(VkDevice device)
{
    VK_CHECK_RESULT(vkResetFences(device, 1, &fence));
}

void MVulkanSemaphore::Create(VkDevice device)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
}

void MVulkanSemaphore::Clean(VkDevice device)
{
    vkDestroySemaphore(device, semaphore, nullptr);
}
