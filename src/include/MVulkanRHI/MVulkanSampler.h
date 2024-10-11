#pragma once
#include "MVulkanDevice.h"

class MVulkanSampler {
public:

	void Create(MVulkanDevice device);
	void Clean(VkDevice device);
	inline VkSampler GetSampler() const { return sampler; }

private:
	VkSampler sampler;
};
