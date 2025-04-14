#include "MVulkanRHI/MVulkanAttachmentBuffer.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"

void MVulkanAttachmentBuffer::Create(MVulkanDevice device, VkExtent2D extent, AttachmentType type, VkFormat format)
{
	m_attachmentType = type;
	m_format = format;

	ImageCreateInfo imageInfo{};
	imageInfo.width = extent.width;
	imageInfo.height = extent.height;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.format = format;
	imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	//imageInfo.samples = device.GetMaxSmaaFlag();
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	ImageViewCreateInfo viewInfo{};
	viewInfo.flag = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.format = format;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

	switch (m_attachmentType) {
	case COLOR_ATTACHMENT:
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		//viewInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.baseMipLevel = 0;
		viewInfo.levelCount = 1;
		viewInfo.baseArrayLayer = 0;
		viewInfo.layerCount = 1;
		viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case DEPTH_STENCIL_ATTACHMENT:
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		viewInfo.flag = VK_IMAGE_ASPECT_DEPTH_BIT;
		break;
	}

	m_texture = std::make_shared<MVulkanTexture>();
	m_texture->Create(device, imageInfo, viewInfo);
	//m_texture->CreateImage(device, imageInfo);
	//m_texture->CreateImageView(viewInfo);

	isActive = true;
}

void MVulkanAttachmentBuffer::Clean()
{
	if(isActive)
		m_texture->Clean();

	isActive = false;
}

VkImage MVulkanAttachmentBuffer::GetImage()const { return m_texture->GetImage(); }

VkImageView MVulkanAttachmentBuffer::GetImageView()const { return m_texture->GetImageView(); }

VkFormat MVulkanAttachmentBuffer::GetFormat() const { return m_format; }

std::shared_ptr<MVulkanTexture> MVulkanAttachmentBuffer::GetTexture() const
{
	return m_texture;
}
