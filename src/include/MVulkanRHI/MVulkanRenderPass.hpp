#pragma once
#ifndef MVULKANRENDERPASS_H
#define MVULKANRENDERPASS_H

#include "MVulkanDevice.hpp"

struct RenderPassFormatsInfo {
	bool useResolvedRef = false;
	bool isFinalRenderPass = false;
	std::vector<VkFormat> imageFormats;
	VkFormat depthFormat;
	VkFormat resolvedFormat;
};

class MVulkanRenderPass {
public:
	MVulkanRenderPass();

	void Create(MVulkanDevice device, VkFormat imageFormat, VkFormat depthFormat, VkFormat swapChainImageFormat, bool isFinalRenderPass = false);
	void Create(MVulkanDevice device, RenderPassFormatsInfo formats);
	void Clean(VkDevice device);

	inline VkRenderPass Get() const { return renderPass; }

private:
	VkRenderPass renderPass;

};


#endif