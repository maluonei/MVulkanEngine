#pragma once
#ifndef MVULKANDESCRIPTOR_H
#define MVULKANDESCRIPTOR_H

#include "vulkan/vulkan_core.h"
#include "vector"
#include "Utils/VulkanUtil.hpp"

struct MVulkanDescriptorImageInfo {
	VkDescriptorImageInfo imageInfo;
	VkDescriptorType samplerType;
};

class MVulkanDescriptorSetAllocator {
public:
	MVulkanDescriptorSetAllocator();
	void Create(VkDevice device);
	void Clean();
	inline VkDescriptorPool Get() {
		return m_descriptorPool;
	}

private:
	VkDevice			m_device;
	VkDescriptorPool	m_descriptorPool;
};


class MVulkanDescriptorSetLayouts {
public:
	MVulkanDescriptorSetLayouts() = default;
	MVulkanDescriptorSetLayouts(VkDescriptorSetLayout _layout);

	void Create(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings);
	void Create(VkDevice device, std::vector<MVulkanDescriptorSetLayoutBinding> bindings);
	void Clean();
	inline VkDescriptorSetLayout Get()const { return m_layout; }
private:
	VkDevice				m_device;
	VkDescriptorSetLayout	m_layout;
};

class MVulkanDescriptorSet {
public:
	MVulkanDescriptorSet() = default;

	void Create(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout);
	void Clean();

	inline VkDescriptorSet Get() { return m_set; }
private:
	VkDevice			m_device;
	VkDescriptorPool	m_pool;
	VkDescriptorSet		m_set;
};

class MVulkanDescriptorSetWrite {
public:
	void Update(
		VkDevice device,
		VkDescriptorSet set,
		std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> combinedImageInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> seperateImageInfos,
		std::vector<VkDescriptorImageInfo> seperateSamplers);

	void Update(
		VkDevice device,
		VkDescriptorSet set,
		std::vector<VkDescriptorBufferInfo> bufferInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> imageInfos);

private:
	VkWriteDescriptorSet m_write;
};
#endif