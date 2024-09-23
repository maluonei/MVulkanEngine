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
    //vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    vkResetCommandBuffer(commandBuffer, 0);
}

void MVulkanCommandList::BeginRenderPass(VkRenderPassBeginInfo* info)
{
    vkCmdBeginRenderPass(commandBuffer, info, VK_SUBPASS_CONTENTS_INLINE);
}

void MVulkanCommandList::EndRenderPass()
{
    vkCmdEndRenderPass(commandBuffer);
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

void MGraphicsCommandList::BindPipeline(VkPipeline pipeline) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void MGraphicsCommandList::SetViewport(uint32_t firstViewport, uint32_t viewportNum, VkViewport* viewport)
{
    vkCmdSetViewport(commandBuffer, firstViewport, viewportNum, viewport);
}

void MGraphicsCommandList::SetScissor(uint32_t firstScissor, uint32_t scissorNum, VkRect2D* scissor)
{
    vkCmdSetScissor(commandBuffer, firstScissor, scissorNum, scissor);
}

void MGraphicsCommandList::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffer, const VkDeviceSize* offset)
{
    vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, buffer, offset);;
}

void MGraphicsCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void MComputeCommandList::BindPipeline(VkPipeline pipeline) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
}


