#include "MVulkanRHI/MVulkanSampler.hpp"


void MVulkanSampler::Create(MVulkanDevice device, MVulkanSamplerCreateInfo info)
{
    m_info = info;

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	
    VkPhysicalDeviceProperties properties = device.GetPhysicalDeviceProperties();

	// ���ù���ģʽ����ַģʽ��
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = info.magFilter;
    samplerInfo.minFilter = info.minFilter;
    samplerInfo.mipmapMode = info.mipMode;
    samplerInfo.minLod = 0.f;
    samplerInfo.maxLod = 4.f;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	vkCreateSampler(device.GetDevice(), &samplerInfo, nullptr, &m_sampler);
}

void MVulkanSampler::Clean(VkDevice device)
{
    vkDestroySampler(device, m_sampler, nullptr);
}