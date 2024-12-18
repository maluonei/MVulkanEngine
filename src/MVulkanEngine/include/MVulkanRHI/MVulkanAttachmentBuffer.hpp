#pragma once
#ifndef MVULKANATTACHMENTBUFFER_H
#define MVULKANATTACHMENTBUFFER_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include <MVulkanRHI/MVulkanDevice.hpp>
#include <MVulkanRHI/MVulkanBuffer.hpp>

class MVulkanAttachmentBuffer {
public:
	MVulkanAttachmentBuffer() = default;
	void Create(MVulkanDevice device, VkExtent2D extent, AttachmentType type, VkFormat format);
	void Clean();

	inline VkImage GetImage()const { return m_image.GetImage(); }
	inline VkImageView GetImageView()const { return m_image.GetImageView(); }

private:

	AttachmentType		m_attachmentType;
	MVulkanImage		m_image;

	bool				isActive = false;
};
#endif

