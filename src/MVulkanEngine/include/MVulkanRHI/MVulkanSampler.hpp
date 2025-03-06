#pragma once
#include "MVulkanDevice.hpp"

struct MVulkanSamplerCreateInfo {
	VkFilter minFilter = VkFilter::VK_FILTER_LINEAR;
	VkFilter magFilter = VkFilter::VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
	float maxLod = 4.f;
	bool anisotropyEnable = true;
};

class MVulkanSampler {
public:

	void Create(MVulkanDevice device, MVulkanSamplerCreateInfo info);
	void Clean();
	inline VkSampler GetSampler() const { return m_sampler; }

private:
	MVulkanSamplerCreateInfo	m_info;
	VkSampler					m_sampler;
	VkDevice					m_device;
};
