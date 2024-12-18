#pragma once
#ifndef MVULKANRENDERPASS_H
#define MVULKANRENDERPASS_H

#include "MVulkanDevice.hpp"

struct RenderPassFormatsInfo {
	bool useResolvedRef = false;
	std::vector<VkFormat> imageFormats;
	VkFormat depthFormat;
	VkFormat resolvedFormat;
	VkImageLayout initialLayout;
	VkImageLayout finalLayout;
	VkImageLayout initialDepthLayout;
	VkImageLayout finalDepthLayout;

	VkAttachmentLoadOp  loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	VkAttachmentLoadOp  depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	VkAttachmentLoadOp  stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
};

class MVulkanRenderPass {
public:
	MVulkanRenderPass();

	void Create(MVulkanDevice device, VkFormat imageFormat, VkFormat depthFormat, VkFormat swapChainImageFormat, VkImageLayout initialLayout, VkImageLayout finalLayout);
	void Create(MVulkanDevice device, RenderPassFormatsInfo formats);
	void Clean();

	inline VkRenderPass Get() const { return m_renderPass; }

private:
	VkRenderPass	m_renderPass;
	VkDevice		m_device;

};


#endif