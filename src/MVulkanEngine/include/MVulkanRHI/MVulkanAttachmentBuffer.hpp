#pragma once
#ifndef MVULKANATTACHMENTBUFFER_H
#define MVULKANATTACHMENTBUFFER_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include <MVulkanRHI/MVulkanDevice.hpp>
#include <MVulkanRHI/MVulkanBuffer.hpp>

class MVulkanAttachmentBuffer {
public:
	void Create(MVulkanDevice device, VkExtent2D extent, AttachmentType type, VkFormat format);
	void Clean(VkDevice device);

	inline VkImage GetImage()const { return image.GetImage(); }
	inline VkImageView GetImageView()const { return image.GetImageView(); }

private:
	AttachmentType attachmentType;
	MVulkanImage image;
};

//class MVulkanDepthBuffer {
//public:
//	void Create(MVulkanDevice device, VkExtent2D extent, VkFormat depthFormat);
//	void Clean(VkDevice device);
//
//	inline VkImage GetImage()const { return depthImage.GetImage(); }
//	inline VkImageView GetImageView()const { return depthImage.GetImageView(); }
//	//static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
//private:
//	MVulkanImage depthImage;
//	//VkImage depthImage;
//	//VkDeviceMemory depthImageMemory;
//	//VkImageView depthImageView;
//};



#endif

