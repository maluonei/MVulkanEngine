#include "MVulkanCommand.h"

MVulkanCommandList::MVulkanCommandList() {

}

MVulkanCommandList::MVulkanCommandList(VkDevice device, const VkCommandListCreateInfo& info)
{
    Create(device, info);
}

void MVulkanCommandList::Begin()
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &begin_info));
}

void MVulkanCommandList::End()
{
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void MVulkanCommandList::Reset()
{
    vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

}

void MVulkanCommandList::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkBufferCopy copyRegion{};
    copyRegion.size = size;

    vkCmdCopyBuffer(
        commandBuffer,
        srcBuffer,
        dstBuffer,
        1,
        &copyRegion
    );
}

void MVulkanCommandList::Create(VkDevice device, const VkCommandListCreateInfo& info)
{
    VkCommandBufferAllocateInfo buffer_alloc_info{};
    commandPool = info.commandPool;
    level = info.level;

    buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_alloc_info.pNext = nullptr;
    buffer_alloc_info.commandPool = commandPool;
    buffer_alloc_info.level = info.level;
    buffer_alloc_info.commandBufferCount = info.commandBufferCount;

    //VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &buffer_alloc_info, &commandBuffer));
    if (vkAllocateCommandBuffers(device, &buffer_alloc_info, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("fail to create commandBuffer");
    }
}

void MVulkanCommandList::Clean(VkDevice _device)
{

}

