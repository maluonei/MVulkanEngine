#include "MVulkanRHI/MVulkanCommand.hpp"

MVulkanCommandList::MVulkanCommandList() {

}

MVulkanCommandList::MVulkanCommandList(VkDevice device, const MVulkanCommandListCreateInfo& info)
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

    VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffer, &begin_info));
}

void MVulkanCommandList::End()
{
    VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffer));
}

void MVulkanCommandList::Reset()
{
    //vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    vkResetCommandBuffer(m_commandBuffer, 0);
}

void MVulkanCommandList::BeginRenderPass(VkRenderPassBeginInfo* info)
{
    vkCmdBeginRenderPass(m_commandBuffer, info, VK_SUBPASS_CONTENTS_INLINE);
}

void MVulkanCommandList::EndRenderPass()
{
    vkCmdEndRenderPass(m_commandBuffer);
}

void MVulkanCommandList::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkBufferCopy copyRegion{};
    copyRegion.size = size;

    vkCmdCopyBuffer(
        m_commandBuffer,
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
        m_commandBuffer,
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
        m_commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
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

    vkCmdCopyBufferToImage(m_commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
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
        m_commandBuffer,
        srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // Դimage���䲼��
        dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // Ŀ��image���䲼��
        1,
        &copyRegion
    );
}

void MVulkanCommandList::BlitImage(VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, std::vector<VkImageBlit> blits, VkFilter filter)
{
    vkCmdBlitImage(m_commandBuffer,
        srcImage, srcLayout,
        dstImage, dstLayout,
        static_cast<uint32_t>(blits.size()), blits.data(),
        filter);
}

void MVulkanCommandList::Create(VkDevice device, const MVulkanCommandListCreateInfo& info)
{
    m_device = device;

    VkCommandBufferAllocateInfo buffer_alloc_info{};
    m_commandPool = info.commandPool;
    m_level = info.level;

    buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_alloc_info.pNext = nullptr;
    buffer_alloc_info.commandPool = m_commandPool;
    buffer_alloc_info.level = info.level;
    buffer_alloc_info.commandBufferCount = info.commandBufferCount;

    if (vkAllocateCommandBuffers(m_device, &buffer_alloc_info, &m_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("fail to create commandBuffer");
    }
}

void MVulkanCommandList::Clean()
{
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
    //vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

//void MVulkanCommandList::BindDescriptorSet(VkPipelineLayout pipelineLayout, uint32_t firstSet, uint32_t descriptorSetCount, VkDescriptorSet* set)
//{
//    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, firstSet, descriptorSetCount, set, 0, nullptr);
//}

void MGraphicsCommandList::BindPipeline(VkPipeline pipeline) {
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void MGraphicsCommandList::SetViewport(uint32_t firstViewport, uint32_t viewportNum, VkViewport* viewport)
{
    vkCmdSetViewport(m_commandBuffer, firstViewport, viewportNum, viewport);
}

void MGraphicsCommandList::SetScissor(uint32_t firstScissor, uint32_t scissorNum, VkRect2D* scissor)
{
    vkCmdSetScissor(m_commandBuffer, firstScissor, scissorNum, scissor);
}

void MGraphicsCommandList::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffer, const VkDeviceSize* offset)
{
    vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, buffer, offset);;
}

void MGraphicsCommandList::BindIndexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer buffer, const VkDeviceSize* offset)
{
    vkCmdBindIndexBuffer(m_commandBuffer, buffer, 0, VK_INDEX_TYPE_UINT32);
}

void MGraphicsCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void MGraphicsCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void MGraphicsCommandList::DrawIndexedIndirectCommand(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    vkCmdDrawIndexedIndirect(m_commandBuffer, buffer, offset, drawCount, stride);
}

void MGraphicsCommandList::BindDescriptorSet(VkPipelineLayout pipelineLayout, uint32_t firstSet, uint32_t descriptorSetCount, VkDescriptorSet* set)
{
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, firstSet, descriptorSetCount, set, 0, nullptr);
}

void MComputeCommandList::BindPipeline(VkPipeline pipeline) {
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
}

void MComputeCommandList::BindDescriptorSet(VkPipelineLayout pipelineLayout, uint32_t firstSet, uint32_t descriptorSetCount, VkDescriptorSet* set)
{
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, firstSet, descriptorSetCount, set, 0, nullptr);
}

void MComputeCommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
}


void MRaytracingCommandList::BindPipeline(VkPipeline pipeline)
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
}

void MRaytracingCommandList::BuildAccelerationStructure(
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> collectedBuildInfo,
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>   collectedRangeInfo)
{
    static auto vkCmdBuildAccelerationStructuresKHR           = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
    vkCmdBuildAccelerationStructuresKHR(m_commandBuffer, collectedBuildInfo.size(), collectedBuildInfo.data(), collectedRangeInfo.data());
}

void MRaytracingCommandList::BindDescriptorSet(VkPipelineLayout pipelineLayout, uint32_t firstSet, uint32_t descriptorSetCount, VkDescriptorSet* set)
{
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, firstSet, descriptorSetCount, set, 0, nullptr);
}

//static auto vkGetAccelerationStructureBuildSizesKHR       = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_device->GetDevice(), "vkGetAccelerationStructureBuildSizesKHR"));
//static auto vkCreateAccelerationStructureKHR              = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device->GetDevice(), "vkCreateAccelerationStructureKHR"));
//static auto vkCmdBuildAccelerationStructuresKHR           = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device->GetDevice(), "vkCmdBuildAccelerationStructuresKHR"));
//static auto vkDestroyAccelerationStructureKHR             = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device->GetDevice(), "vkDestroyAccelerationStructureKHR"));
//static auto vkCmdCopyAccelerationStructureKHR             = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device->GetDevice(), "vkCmdCopyAccelerationStructureKHR"));
//static auto vkCmdWriteAccelerationStructuresPropertiesKHR = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(m_device->GetDevice(), "vkCmdWriteAccelerationStructuresPropertiesKHR"));
//VK_CHECK_NULLPTR(vkGetAccelerationStructureBuildSizesKHR, "RHIBuildRayTracingBLAS: vkGetAccelerationStructureBuildSizesKHR is nullptr", return);
//VK_CHECK_NULLPTR(vkCreateAccelerationStructureKHR, "RHIBuildRayTracingBLAS: vkCreateAccelerationStructureKHR is nullptr", return);
//VK_CHECK_NULLPTR(vkCmdBuildAccelerationStructuresKHR, "RHIBuildRayTracingBLAS: vkCmdBuildAccelerationStructuresKHR is nullptr", return);
//VK_CHECK_NULLPTR(vkDestroyAccelerationStructureKHR, "RHIBuildRayTracingBLAS: vkDestroyAccelerationStructureKHR is nullptr", return);
//VK_CHECK_NULLPTR(vkCmdCopyAccelerationStructureKHR, "RHIBuildRayTracingBLAS: vkCmdCopyAccelerationStructureKHR is nullptr", return);
//VK_CHECK_NULLPTR(vkCmdWriteAccelerationStructuresPropertiesKHR, "RHIBuildRayTracingBLAS: vkCmdWriteAccelerationStructuresPropertiesKHR is nullptr", return);


