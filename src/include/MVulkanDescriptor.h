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
		return descriptorPools[type];
	}

private:
	std::vector<VkDescriptorPool> descriptorPools;
};


class MVulkanDescriptorSetLayouts {
public:

private:

};
#endif