#include "MVulkanRHI/MVulkanFrameBuffer.hpp"
#include <stdexcept>
#include <array>

MVulkanFrameBuffer::MVulkanFrameBuffer()
{
}

void MVulkanFrameBuffer::Create(VkDevice device, FrameBufferCreateInfo creatInfo)
{
    std::array<VkImageView, 2> attachments = {
        creatInfo.swapChainImageViews,
        creatInfo.depthImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = creatInfo.renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
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
