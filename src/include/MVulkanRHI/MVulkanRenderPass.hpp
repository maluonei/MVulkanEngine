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
	VkImageLayout finalDepthLayout;
};

class MVulkanRenderPass {
public:
	MVulkanRenderPass();

	void Create(MVulkanDevice device, VkFormat imageFormat, VkFormat depthFormat, VkFormat swapChainImageFormat, VkImageLayout initialLayout, VkImageLayout finalLayout);
	void Create(MVulkanDevice device, RenderPassFormatsInfo formats);
	void Clean(VkDevice device);

	inline VkRenderPass Get() const { return renderPass; }

private:
	VkRenderPass renderPass;

};


#endif