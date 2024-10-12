#pragma once
#include "MVulkanDevice.hpp"

class MVulkanSampler {
public:

	void Create(MVulkanDevice device);
	void Clean(VkDevice device);
	inline VkSampler GetSampler() const { return sampler; }

private:
	VkSampler sampler;
};
