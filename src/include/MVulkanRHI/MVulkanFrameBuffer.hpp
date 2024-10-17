#pragma once
#ifndef MVULKANFRAMEBUFFER_H
#define MVULKANFRAMEBUFFER_H

#include "vulkan/vulkan_core.h"
#include <vector>
#include <MVulkanRHI/MVulkanAttachmentBuffer.hpp>

struct FrameBufferCreateInfo {
	//VkImageView colorImageView;
	//VkImageView depthImageView;
	//uint16_t numAttachments;
	
	
	VkRenderPass renderPass;
	VkExtent2D extent;
	uint16_t numAttachments;
	VkFormat* imageAttachmentFormats = nullptr;
};

class MVulkanFrameBuffer {
public:
	MVulkanFrameBuffer();

	void Create(MVulkanDevice device, FrameBufferCreateInfo creatInfo);
	void Clean(VkDevice device);

	inline VkImage GetImage(int i) const {
		if (i < 0 || i >= colorBuffers.size()) {
			spdlog::error("invalid index:", i);
		}
		return colorBuffers[i].GetImage();
	}

	inline VkImage GetDepthImage() const { return depthBuffer.GetImage(); }

	inline VkFramebuffer Get() const { return frameBuffer; }

private:
	VkFramebuffer frameBuffer;

	std::vector<MVulkanAttachmentBuffer> colorBuffers;
	MVulkanAttachmentBuffer depthBuffer;
};


#endif