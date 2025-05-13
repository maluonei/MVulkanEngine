#include "MVulkanRHI/MVulkanFrameBuffer.hpp"
#include <stdexcept>
#include <array>

MVulkanFrameBuffer::MVulkanFrameBuffer()
{
}

std::shared_ptr<MVulkanTexture> MVulkanFrameBuffer::GetDepthTexture() const {
    return m_depthBuffer.GetTexture();
}

std::shared_ptr<MVulkanTexture> MVulkanFrameBuffer::GetTexture(int i) const {
    return m_colorBuffers[i].GetTexture();
}

VkImageView MVulkanFrameBuffer::GetDepthImageView() const {
    if (m_info.depthView) return m_info.depthView;
    else return m_depthBuffer.GetImageView();
}

void MVulkanFrameBuffer::Create(MVulkanDevice device, FrameBufferCreateInfo creatInfo)
{
    m_device = device.GetDevice();
    m_info = creatInfo;

    m_colorBuffers.resize(creatInfo.numAttachments);
    std::vector<VkImageView> attachments(creatInfo.numAttachments);
    if(creatInfo.useDepthBuffer)    
        attachments.resize(creatInfo.numAttachments + 1);


    if (creatInfo.useSwapchainImageViews) {
        m_swapChain = creatInfo.swapChain;
        //assert(creatInfo.numAttachments == 1);
        for (auto i = 0; i < creatInfo.numAttachments; i++) {
            if (i == 0) {
                attachments[i] = m_swapChain.GetImageView(creatInfo.swapChainImageIndex);
            }
            else{
                m_colorBuffers[i].Create(device, creatInfo.extent, COLOR_ATTACHMENT, creatInfo.imageAttachmentFormats[i]);
                attachments[i] = m_colorBuffers[i].GetImageView();
            }
        }
    }
    else {
        for (auto i = 0; i < creatInfo.numAttachments; i++) {
            m_colorBuffers[i].Create(device, creatInfo.extent, COLOR_ATTACHMENT, creatInfo.imageAttachmentFormats[i]);
            attachments[i] = m_colorBuffers[i].GetImageView();
        }
    }

    if (creatInfo.useDepthBuffer) {
        if (creatInfo.depthView) {
            attachments[creatInfo.numAttachments] = creatInfo.depthView;
        }
        else {
            m_depthBuffer.Create(device, creatInfo.extent, DEPTH_STENCIL_ATTACHMENT, m_info.depthStencilFormat);
            attachments[creatInfo.numAttachments] = m_depthBuffer.GetImageView();
        }
    }

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

    VK_CHECK_RESULT(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_frameBuffer));
    //if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_frameBuffer) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create framebuffer!");
    //}
}

void MVulkanFrameBuffer::Clean()
{
    if (!m_info.useSwapchainImageViews) {
        for (auto i = 0; i < m_colorBuffers.size(); i++) {
            m_colorBuffers[i].Clean();
        }
    }
    m_depthBuffer.Clean();
    vkDestroyFramebuffer(m_device, m_frameBuffer, nullptr);
}
