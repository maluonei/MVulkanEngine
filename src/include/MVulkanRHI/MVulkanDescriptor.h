#pragma once
#ifndef MVULKANDESCRIPTOR_H
#define MVULKANDESCRIPTOR_H

#include "vulkan/vulkan_core.h"
#include "vector"
#include "Utils/VulkanUtil.h"

class MVulkanDescriptorSetAllocator {
public:
	MVulkanDescriptorSetAllocator();
	void Create(VkDevice device);
	inline VkDescriptorPool Get(DescriptorType type) {
		//if (type >= descriptorPools.size()) {
		//	return nullptr;
		//}
		return descriptorPools[type];
	}

private:
	std::vector<VkDescriptorPool> descriptorPools;
};


class MVulkanDescriptorSetLayouts {
public:

	void Create(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings);
	inline VkDescriptorSetLayout Get()const { return layout; }
private:
	VkDescriptorSetLayout layout;
};

class MVulkanDescriptorSet {
public:

	void Create(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, int _MAX_FRAMES_IN_FLIGHT);
	inline std::vector<VkDescriptorSet> Get() { return sets; }
private:
	std::vector<VkDescriptorSet> sets;
};

class MVulkanDescriptorSetWrite {
public:
	
	void Update(VkDevice device, std::vector<VkDescriptorSet> sets, std::vector<VkDescriptorBufferInfo> infos, int _MAX_FRAMES_IN_FLIGHT);

private:
	VkWriteDescriptorSet write;
};
#endif