#pragma once
#ifndef MVULKANRENDERPASS_H
#define MVULKANRENDERPASS_H

#include "vulkan/vulkan_core.h"

class MVulkanrRenderPass {
public:
	MVulkanrRenderPass();

	void Create(VkDevice device, VkFormat imageFormat);
	void Clean(VkDevice device);

	inline VkRenderPass Get() const { return renderPass; }

private:
	VkRenderPass renderPass;

};


#endif