#include "Utils/VulkanUtil.hpp"

VkDescriptorType ResourceType2VkDescriptorType(const ResourceType type)
{
    switch (type)
    {
    case ResourceType_ConstantBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case ResourceType_StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case ResourceType_SampledImage:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case ResourceType_StorageImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case ResourceType_CombinedImageSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case ResourceType_Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case ResourceType_AccelerationStructure:
        return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    default:
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

VkRenderingInfo RenderingInfo2VkRenderingInfo(CONST RenderingInfo& info) {
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    VkRenderingAttachmentInfo depthAttachments;

    for (auto i = 0; i < info.colorAttachments.size(); i++) {
        auto attachment = info.colorAttachments[i];

        VkRenderingAttachmentInfo ainfo;
        ainfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        ainfo.imageView = attachment.view;
        ainfo.imageLayout = attachment.layout;
        ainfo.loadOp = attachment.loadOp;
        ainfo.storeOp = attachment.storeOp;
        ainfo.clearValue.color = { {attachment.clearColor.x, attachment.clearColor.y, attachment.clearColor.z, attachment.clearColor.w} };

        colorAttachments.push_back(ainfo);
    }

    {
        auto depthAttachment = info.depthAttachment;

        depthAttachments.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachments.imageView = depthAttachment.view;
        depthAttachments.imageLayout = depthAttachment.layout;
        depthAttachments.loadOp = depthAttachment.loadOp;
        depthAttachments.storeOp = depthAttachment.storeOp;
        depthAttachments.clearValue.color = { {depthAttachment.clearColor.x, depthAttachment.clearColor.y, depthAttachment.clearColor.z, depthAttachment.clearColor.w} };
    }

    VkRenderingInfo renderingInfo;
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = info.offset;
    renderingInfo.renderArea.extent = info.extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = colorAttachments.size();
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = &depthAttachments;

    return renderingInfo;
}

bool IsDepthFormat(VkFormat format)
{
    if (
        format == VK_FORMAT_D16_UNORM ||
        format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
        format == VK_FORMAT_D32_SFLOAT ||
        format == VK_FORMAT_D16_UNORM_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT
        )
        return true;

    return false;
}

//void RenderingInfo::RenderingInfo2VkRenderingInfo()
//{
//    std::vector<VkRenderingAttachmentInfo> _colorAttachments;
//    VkRenderingAttachmentInfo depthAttachments;
//
//    for (auto i = 0; i < this->colorAttachments.size(); i++) {
//        auto attachment = this->colorAttachments[i];
//
//        VkRenderingAttachmentInfo ainfo;
//        ainfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
//        ainfo.imageView = attachment.view;
//        ainfo.imageLayout = attachment.layout;
//        ainfo.loadOp = attachment.loadOp;
//        ainfo.storeOp = attachment.storeOp;
//        ainfo.clearValue.color = { {attachment.clearColor.x, attachment.clearColor.y, attachment.clearColor.z, attachment.clearColor.w} };
//
//        _colorAttachments.push_back(ainfo);
//    }
//
//    {
//        auto depthAttachment = info.depthAttachment;
//
//        depthAttachments.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
//        depthAttachments.imageView = depthAttachment.view;
//        depthAttachments.imageLayout = depthAttachment.layout;
//        depthAttachments.loadOp = depthAttachment.loadOp;
//        depthAttachments.storeOp = depthAttachment.storeOp;
//        depthAttachments.clearValue.color = { {depthAttachment.clearColor.x, depthAttachment.clearColor.y, depthAttachment.clearColor.z, depthAttachment.clearColor.w} };
//    }
//
//    VkRenderingInfo renderingInfo;
//    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
//    renderingInfo.renderArea.offset = info.offset;
//    renderingInfo.renderArea.extent = info.extent;
//    renderingInfo.layerCount = 1;
//    renderingInfo.colorAttachmentCount = colorAttachments.size();
//    renderingInfo.pColorAttachments = colorAttachments.data();
//    renderingInfo.pDepthAttachment = &depthAttachments;
//
//    return renderingInfo;
//}
