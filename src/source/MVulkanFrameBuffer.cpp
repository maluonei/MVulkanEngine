#include "MVulkanFrameBuffer.h"
#include <stdexcept>

MVulkanFrameBuffer::MVulkanFrameBuffer()
{
}

void MVulkanFrameBuffer::Create(VkDevice device, FrameBufferCreateInfo creatInfo)
{
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = creatInfo.renderPass;
    framebufferInfo.attachmentCount = creatInfo.numAttachments;
    framebufferInfo.pAttachments = creatInfo.attachments;
    framebufferInfo.width = creatInfo.extent.width;
    framebufferInfo.height = creatInfo.extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

void MVulkanFrameBuffer::Clean(VkDevice device)
{
    vkDestroyFramebuffer(device, frameBuffer, nullptr);
}
