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

	VkRenderPass renderPass;
	VkExtent2D extent;
	uint16_t numAttachments;
	VkFormat depthStencilFormat;
	VkFormat* imageAttachmentFormats = nullptr;

	MVulkanSwapchain swapChain;
	uint32_t swapChainImageIndex;
	VkImageView* colorAttachmentResolvedViews = nullptr;

	VkImageView depthView = nullptr;
};

class MVulkanFrameBuffer {
public:
	MVulkanFrameBuffer();

	void Create(MVulkanDevice device, FrameBufferCreateInfo creatInfo);
	void Clean();

	inline VkImage GetImage(int i) const {
		if (i < 0 || i >= m_colorBuffers.size()) {
			spdlog::error("invalid index:", i);
		}

		if (m_info.useSwapchainImageViews) {
			return m_swapChain.GetImage(i);
		}
		
		return m_colorBuffers[i].GetImage();
	}

	inline VkImageView GetImageView(int i) const {
		if (i < 0 || i >= m_colorBuffers.size()) {
			spdlog::error("invalid index:", i);
		}
		if (m_info.useSwapchainImageViews) {
			return m_swapChain.GetImageView(i);
		}

		return m_colorBuffers[i].GetImageView();
	}

	inline VkImage GetDepthImage() const { return m_depthBuffer.GetImage(); }

	VkImageView GetDepthImageView() const;

	inline VkFramebuffer Get() const { return m_frameBuffer; }

	inline const uint32_t ColorAttachmentsCount() const { return static_cast<uint32_t>(m_colorBuffers.size()); }

	inline const VkExtent2D GetExtent2D()const { return m_info.extent; }

private:
	VkDevice					m_device;
	FrameBufferCreateInfo		m_info;
	MVulkanSwapchain			m_swapChain;

	VkFramebuffer				m_frameBuffer;

	std::vector<MVulkanAttachmentBuffer> m_colorBuffers;
	MVulkanAttachmentBuffer		m_depthBuffer;
};

#endif