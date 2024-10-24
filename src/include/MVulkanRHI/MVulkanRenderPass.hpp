#pragma once
#ifndef MVULKANRENDERPASS_H
#define MVULKANRENDERPASS_H

#include "MVulkanDevice.hpp"

class MVulkanrRenderPass {
public:
	MVulkanrRenderPass();

	void Create(MVulkanDevice device, VkFormat imageFormat, VkFormat depthFormat, VkFormat swapChainImageFormat);
	void Clean(VkDevice device);

	inline VkRenderPass Get() const { return renderPass; }

private:
	VkRenderPass renderPass;

};


#endif