#pragma once
#ifndef MVULKANATTACHMENTBUFFER_H
#define MVULKANATTACHMENTBUFFER_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include <MVulkanRHI/MVulkanDevice.hpp>


class MVulkanTexture;

class MVulkanAttachmentBuffer {
public:
	MVulkanAttachmentBuffer() = default;
	void Create(MVulkanDevice device, VkExtent2D extent, AttachmentType type, VkFormat format);
	void Clean();

	VkImage GetImage()const;
	VkImageView GetImageView()const;
	VkFormat GetFormat() const;
	std::shared_ptr<MVulkanTexture> GetTexture() const;
private:

	AttachmentType		m_attachmentType;
	//MVulkanImage		m_image;
	std::shared_ptr<MVulkanTexture> m_texture;
	VkFormat			m_format;

	bool				isActive = false;
};
#endif

