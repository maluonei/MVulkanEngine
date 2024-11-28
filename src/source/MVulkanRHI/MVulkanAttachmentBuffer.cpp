#include "MVulkanRHI/MVulkanAttachmentBuffer.hpp"

//void MVulkanDepthBuffer::Create(MVulkanDevice device, VkExtent2D extent, VkFormat depthFormat)
//{
//	//createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
//	//depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
//
//	ImageCreateInfo imageInfo{};
//	imageInfo.width = extent.width;
//	imageInfo.height = extent.height;
//	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
//	imageInfo.format = depthFormat;
//	imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//	depthImage.CreateImage(device, imageInfo);
//
//	ImageViewCreateInfo viewInfo{};
//	viewInfo.flag = VK_IMAGE_ASPECT_DEPTH_BIT;
//	viewInfo.format = depthFormat;
//	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//	depthImage.CreateImageView(device, viewInfo);
//}
//
//void MVulkanDepthBuffer::Clean(VkDevice device)
//{
//	depthImage.Clean(device);
//}

void MVulkanAttachmentBuffer::Create(MVulkanDevice device, VkExtent2D extent, AttachmentType type, VkFormat format)
{
	attachmentType = type;

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

	switch (attachmentType) {
	case COLOR_ATTACHMENT:
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		viewInfo.flag = VK_IMAGE_ASPECT_DEPTH_BIT;
		break;
	}

	image.CreateImage(device, imageInfo);
	image.CreateImageView(device, viewInfo);
}

void MVulkanAttachmentBuffer::Clean(VkDevice device)
{
	image.Clean(device);
}
