#include "MVulkanRHI/MVulkanCommand.h"

MVulkanCommandAllocator::MVulkanCommandAllocator()
{
}

void MVulkanCommandAllocator::Create(MVulkanDevice device)
{
    if (commandPools.size() == 0) {
        QueueType queueTypes[] = { GRAPHICS_QUEUE, COMPUTE_QUEUE, TRANSFER_QUEUE, PRESENT_QUEUE };

        for (auto queueType : queueTypes) {
            VkCommandPool commandPool;

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = device.GetQueueFamilyIndices(queueType);

            if (vkCreateCommandPool(device.GetDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create command pool!");
            }

            commandPools.insert({ queueType , commandPool });
        }
    }
}

void MVulkanCommandAllocator::Clean(VkDevice device)
{
    for (auto pair : commandPools) {
        vkDestroyCommandPool(device, pair.second, nullptr);
    }
    commandPools.clear();
}
