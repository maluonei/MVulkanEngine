#pragma once
#ifndef MVULKANFRAMEBUFFER_H
#define MVULKANFRAMEBUFFER_H

#include "vulkan/vulkan_core.h"
#include <vector>
#include "MVulkanRHI/MVulkanAttachmentBuffer.hpp"
#include "MVulkanRHI/MVulkanSwapchain.hpp"

struct FrameBufferCreateInfo {
	bool useAttachmentResolve = false;
	bool useSwapchainImageViews = false;
	//bool finalFrameBuffer = false;
	VkRenderPass renderPass;
	VkExtent2D extent;
	uint16_t numAttachments;
	VkFormat depthStencilFormat;
	VkFormat* imageAttachmentFormats = nullptr;

	MVulkanSwapchain swapChain;
	uint32_t swapChainImageIndex;
	//VkImageView* swapchainImageViews = nullptr;
	//VkImage* swapchainImages = nullptr;
	VkImageView* colorAttachmentResolvedViews = nullptr;

	//bool reuseDepthView = false;
	VkImageView depthView = nullptr;
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

		if (m_info.useSwapchainImageViews) {
			return m_swapChain.GetImage(i);
		}
		
		return colorBuffers[i].GetImage();
	}

	inline VkImageView GetImageView(int i) const {
		if (i < 0 || i >= colorBuffers.size()) {
			spdlog::error("invalid index:", i);
		}
		if (m_info.useSwapchainImageViews) {
			return m_swapChain.GetImageView(i);
		}

		return colorBuffers[i].GetImageView();
	}

	inline VkImage GetDepthImage() const { return depthBuffer.GetImage(); }

	inline VkImageView GetDepthImageView() const { return depthBuffer.GetImageView(); }

	inline VkFramebuffer Get() const { return frameBuffer; }

	inline const uint32_t ColorAttachmentsCount() const { return static_cast<uint32_t>(colorBuffers.size()); }

	inline const VkExtent2D GetExtent2D()const { return m_info.extent; }

private:
	FrameBufferCreateInfo m_info;
	MVulkanSwapchain m_swapChain;

	VkFramebuffer frameBuffer;

	std::vector<MVulkanAttachmentBuffer> colorBuffers;
	MVulkanAttachmentBuffer depthBuffer;
};

#endif