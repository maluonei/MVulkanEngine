#include "MVulkanRHI/MVulkanSyncObject.hpp"
#include "Utils/VulkanUtil.hpp"

MVulkanFence::~MVulkanFence()
{
}

void MVulkanFence::Create(VkDevice device)
{
    m_device = device;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &m_fence));
}

void MVulkanFence::Clean()
{
    vkDestroyFence(m_device, m_fence, nullptr);
}

void MVulkanFence::WaitForSignal()
{
    VK_CHECK_RESULT(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX));
}

void MVulkanFence::Reset()
{
    VK_CHECK_RESULT(vkResetFences(m_device, 1, &m_fence));
}

void MVulkanSemaphore::Create(VkDevice device)
{
    m_device = device;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_semaphore));
}

void MVulkanSemaphore::Clean()
{
    vkDestroySemaphore(m_device, m_semaphore, nullptr);
}
