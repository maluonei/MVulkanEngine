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
	
	bool useAttachmentResolve = false;
	bool finalFrameBuffer = false;
	VkRenderPass renderPass;
	VkExtent2D extent;
	uint16_t numAttachments;
	VkFormat* imageAttachmentFormats = nullptr;

	VkImageView* colorAttachmentResolvedViews;
	//VkImageView swapChainImageView;
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

	inline VkImageView GetImageView(int i) const {
		if (i < 0 || i >= colorBuffers.size()) {
			spdlog::error("invalid index:", i);
		}
		return colorBuffers[i].GetImageView();
	}

	inline VkImage GetDepthImage() const { return depthBuffer.GetImage(); }

	inline VkFramebuffer Get() const { return frameBuffer; }

private:
	VkFramebuffer frameBuffer;

	std::vector<MVulkanAttachmentBuffer> colorBuffers;
	MVulkanAttachmentBuffer depthBuffer;
};


#endif