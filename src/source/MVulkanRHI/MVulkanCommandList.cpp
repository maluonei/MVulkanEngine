#include "MVulkanRHI/MVulkanCommand.hpp"

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

void MVulkanCommandList::TransitionImageLayout(std::vector<MVulkanImageMemoryBarrier> _barriers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
    std::vector<VkImageMemoryBarrier> barriers;

    for (auto i = 0; i < _barriers.size(); i++) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = _barriers[i].oldLayout;
        barrier.newLayout = _barriers[i].newLayout;
        barrier.srcQueueFamilyIndex = _barriers[i].srcQueueFamilyIndex;
        barrier.dstQueueFamilyIndex = _barriers[i].dstQueueFamilyIndex;
        barrier.image = _barriers[i].image;
        barrier.subresourceRange.aspectMask = _barriers[i].aspectMask;
        barrier.subresourceRange.baseMipLevel = _barriers[i].baseMipLevel;
        barrier.subresourceRange.levelCount = _barriers[i].levelCount;
        barrier.subresourceRange.baseArrayLayer = _barriers[i].baseArrayLayer;
        barrier.subresourceRange.layerCount = _barriers[i].layerCount;

        barriers.push_back(barrier);
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        barriers.size(), barriers.data()
    );
}

void MVulkanCommandList::TransitionImageLayout(MVulkanImageMemoryBarrier _barrier, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = _barrier.oldLayout;
    barrier.newLayout = _barrier.newLayout;
    barrier.srcQueueFamilyIndex = _barrier.srcQueueFamilyIndex;
    barrier.dstQueueFamilyIndex = _barrier.dstQueueFamilyIndex;
    barrier.image = _barrier.image;
    barrier.subresourceRange.aspectMask = _barrier.aspectMask;
    barrier.subresourceRange.baseMipLevel = _barrier.baseMipLevel;
    barrier.subresourceRange.levelCount = _barrier.levelCount;
    barrier.subresourceRange.baseArrayLayer = _barrier.baseArrayLayer;
    barrier.subresourceRange.layerCount = _barrier.layerCount;

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    //endSingleTimeCommands(commandBuffer);
    //vkEndCommandBuffer(commandBuffer);
}

void MVulkanCommandList::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, unsigned int width, unsigned int height, uint32_t bufferOffset, uint32_t mipLevel, uint32_t baseArrayLayer)
//void MVulkanCommandList::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, unsigned int width, unsigned int height, uint32_t mipLevel, uint32_t baseArrayLayer)
{
    VkBufferImageCopy region{};
    region.bufferOffset = bufferOffset;
    //region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = baseArrayLayer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void MVulkanCommandList::CopyImage(VkImage srcImage, VkImage dstImage, unsigned int width, unsigned int height, MVulkanImageCopyInfo copyInfo)
{
    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = copyInfo.srcAspectMask;
    copyRegion.srcSubresource.mipLevel = copyInfo.srcMipLevel;
    copyRegion.srcSubresource.baseArrayLayer = copyInfo.srcArrayLayer;
    copyRegion.srcSubresource.layerCount = copyInfo.layerCount;
    copyRegion.srcOffset = { 0, 0, 0 };

    copyRegion.dstSubresource.aspectMask = copyInfo.dstAspectMask;
    copyRegion.dstSubresource.mipLevel = copyInfo.dstMipLevel;
    copyRegion.dstSubresource.baseArrayLayer = copyInfo.dstArrayLayer;
    copyRegion.dstSubresource.layerCount = copyInfo.layerCount;
    copyRegion.dstOffset = { 0, 0, 0 };

    copyRegion.extent.width = width;    // ���ƵĿ���
    copyRegion.extent.height = height;  // ���Ƶĸ߶�
    copyRegion.extent.depth = 1;

    vkCmdCopyImage(
        commandBuffer,
        srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // Դimage���䲼��
        dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // Ŀ��image���䲼��
        1,
        &copyRegion
    );
}

void MVulkanCommandList::BlitImage(VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, std::vector<VkImageBlit> blits, VkFilter filter)
{
    vkCmdBlitImage(commandBuffer,
        srcImage, srcLayout,
        dstImage, dstLayout,
        static_cast<uint32_t>(blits.size()), blits.data(),
        filter);
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

void MGraphicsCommandList::BindIndexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer buffer, const VkDeviceSize* offset)
{
    vkCmdBindIndexBuffer(commandBuffer, buffer, 0, VK_INDEX_TYPE_UINT32);
}

void MGraphicsCommandList::BindDescriptorSet(VkPipelineLayout pipelineLayout, uint32_t firstSet, uint32_t descriptorSetCount, VkDescriptorSet* set)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, firstSet, descriptorSetCount, set, 0, nullptr);
}

void MGraphicsCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void MGraphicsCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void MGraphicsCommandList::DrawIndexedIndirectCommand(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

void MComputeCommandList::BindPipeline(VkPipeline pipeline) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
}


