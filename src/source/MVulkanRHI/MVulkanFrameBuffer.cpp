#include "MVulkanRHI/MVulkanFrameBuffer.hpp"
#include <stdexcept>
#include <array>

MVulkanFrameBuffer::MVulkanFrameBuffer()
{
}

void MVulkanFrameBuffer::Create(MVulkanDevice device, FrameBufferCreateInfo creatInfo)
{
    m_info = creatInfo;

    colorBuffers.resize(creatInfo.numAttachments);
    std::vector<VkImageView> attachments;
    attachments.resize(creatInfo.numAttachments + 1);

    if (creatInfo.useSwapchainImageViews) {
        m_swapChain = creatInfo.swapChain;
        assert(creatInfo.numAttachments == 1);
        for (auto i = 0; i < creatInfo.numAttachments; i++) {
            attachments[i] = m_swapChain.GetImageView(creatInfo.swapChainImageIndex);
        }
    }
    else {
        for (auto i = 0; i < creatInfo.numAttachments; i++) {
            colorBuffers[i].Create(device, creatInfo.extent, COLOR_ATTACHMENT, creatInfo.imageAttachmentFormats[i]);
            attachments[i] = colorBuffers[i].GetImageView();
        }
    }

    depthBuffer.Create(device, creatInfo.extent, DEPTH_STENCIL_ATTACHMENT, device.FindDepthFormat());
    attachments[creatInfo.numAttachments] = depthBuffer.GetImageView();

    if(creatInfo.useAttachmentResolve)
    {
        for(auto i=0;i<creatInfo.numAttachments;i++)
            attachments.push_back(creatInfo.colorAttachmentResolvedViews[i]);
    }
        
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = creatInfo.renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = creatInfo.extent.width;
    framebufferInfo.height = creatInfo.extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device.GetDevice(), &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

void MVulkanFrameBuffer::Clean(VkDevice device)
{
    for (auto i = 0; i < colorBuffers.size(); i++) {
        colorBuffers[i].Clean(device);
    }
    depthBuffer.Clean(device);
    vkDestroyFramebuffer(device, frameBuffer, nullptr);
}
