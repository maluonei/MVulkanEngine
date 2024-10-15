#pragma once
#ifndef MVULKANDEPTHBUFFER_H
#define MVULKANDEPTHBUFFER_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include <MVulkanRHI/MVulkanDevice.hpp>
#include <MVulkanRHI/MVulkanBuffer.hpp>

class MVulkanDepthBuffer {
public:

	void Create(MVulkanDevice device, VkExtent2D extent, VkFormat depthFormat);
	//static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
private:
	MVulkanImage depthImage;
	//VkImage depthImage;
	//VkDeviceMemory depthImageMemory;
	//VkImageView depthImageView;
};



#endif

