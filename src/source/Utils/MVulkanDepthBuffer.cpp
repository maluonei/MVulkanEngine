#include "MVulkanRHI/MVulkanDepthBuffer.hpp"

void MVulkanDepthBuffer::Create(MVulkanDevice device, VkExtent2D extent, VkFormat depthFormat)
{
	//createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	//depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	ImageCreateInfo imageInfo{};
	imageInfo.width = extent.width;
	imageInfo.height = extent.height;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.format = depthFormat;
	imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depthImage.CreateImage(device, imageInfo);

	ImageViewCreateInfo viewInfo{};
	viewInfo.flag = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.format = depthFormat;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthImage.CreateImageView(device, viewInfo);
}

void MVulkanDepthBuffer::Clean(VkDevice device)
{
	depthImage.Clean(device);
}
