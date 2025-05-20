#include "MVulkanRHI/MVulkanCommand.hpp"

MVulkanCommandAllocator::MVulkanCommandAllocator()
{
}

void MVulkanCommandAllocator::Create(MVulkanDevice device)
{
    m_device = device.GetDevice();

    if (m_commandPools.size() == 0) {
        QueueType queueTypes[] = {
            GRAPHICS_QUEUE, 
            COMPUTE_QUEUE, 
            TRANSFER_QUEUE, 
            PRESENT_QUEUE,
            GENERAL_QUEUE
        };

        for (auto queueType : queueTypes) {
            VkCommandPool commandPool;

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = device.GetQueueFamilyIndices(queueType);

            if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create command pool!");
            }

            m_commandPools.insert({ queueType , commandPool });
        }
    }
}

void MVulkanCommandAllocator::Clean()
{
    for (auto pair : m_commandPools) {
        vkDestroyCommandPool(m_device, pair.second, nullptr);
    }
    m_commandPools.clear();
}
