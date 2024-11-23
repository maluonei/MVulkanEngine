#pragma once
#ifndef MVULKANDESCRIPTOR_H
#define MVULKANDESCRIPTOR_H

#include "vulkan/vulkan_core.h"
#include "vector"
#include "Utils/VulkanUtil.hpp"

class MVulkanDescriptorSetAllocator {
public:
	MVulkanDescriptorSetAllocator();
	void Create(VkDevice device);
	inline VkDescriptorPool Get() {
		//if (type >= descriptorPools.size()) {
		//	return nullptr;
		//}
		//return descriptorPools[type];
		return descriptorPool;
	}

private:
	//std::vector<VkDescriptorPool> descriptorPools;
	VkDescriptorPool descriptorPool;
};


class MVulkanDescriptorSetLayouts {
public:
	MVulkanDescriptorSetLayouts() = default;
	MVulkanDescriptorSetLayouts(VkDescriptorSetLayout _layout);

	void Create(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings);
	void Create(VkDevice device, std::vector<MVulkanDescriptorSetLayoutBinding> bindings);
	inline VkDescriptorSetLayout Get()const { return layout; }
private:
	VkDescriptorSetLayout layout;
};

class MVulkanDescriptorSet {
public:
	MVulkanDescriptorSet() = default;
	MVulkanDescriptorSet(VkDescriptorSet _set);

	void Create(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout);

	static std::vector<MVulkanDescriptorSet> CreateDescriptorSets(VkDevice device, VkDescriptorPool pool, std::vector<VkDescriptorSetLayout> layouts);

	inline VkDescriptorSet Get() { return set; }
private:
	VkDescriptorSet set;
};

class MVulkanDescriptorSetWrite {
public:
	void Update(
		VkDevice device,
		VkDescriptorSet set,
		std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> imageInfos);

	void Update(
		VkDevice device,
		VkDescriptorSet set,
		std::vector<VkDescriptorBufferInfo> bufferInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> imageInfos);

private:
	VkWriteDescriptorSet write;
};
#endif