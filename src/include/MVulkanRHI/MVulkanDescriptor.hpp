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
	void Create(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings);
	void Create(VkDevice device, std::vector<MVulkanDescriptorSetLayoutBinding> bindings);
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
	void Update(
		VkDevice device, 
		std::vector<VkDescriptorSet> sets, 
		std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos, 
		std::vector<std::vector<VkDescriptorImageInfo>> imageInfos, 
		int _MAX_FRAMES_IN_FLIGHT);

private:
	VkWriteDescriptorSet write;
};
#endif