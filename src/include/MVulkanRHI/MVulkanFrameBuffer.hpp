#pragma once
#ifndef MVULKANFRAMEBUFFER_H
#define MVULKANFRAMEBUFFER_H

#include "vulkan/vulkan_core.h"
#include <vector>

struct FrameBufferCreateInfo {
	VkImageView swapChainImageViews;
	VkImageView depthImageView;
	uint16_t numAttachments;
	VkRenderPass renderPass;
	VkExtent2D extent;
};

class MVulkanFrameBuffer {
public:
	MVulkanFrameBuffer();

	void Create(VkDevice device, FrameBufferCreateInfo creatInfo);
	void Clean(VkDevice device);

	inline VkFramebuffer Get() const { return frameBuffer; }

private:
	VkFramebuffer frameBuffer;


};


#endif