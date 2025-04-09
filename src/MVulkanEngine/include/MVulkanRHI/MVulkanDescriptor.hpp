#pragma once
#ifndef MVULKANDESCRIPTOR_H
#define MVULKANDESCRIPTOR_H

#include "vulkan/vulkan_core.h"
#include <vector>
#include <variant>
#include "Utils/VulkanUtil.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"

enum ResourceType
{
	ResourceType_ConstantBuffer,
	ResourceType_StorageBuffer,
	ResourceType_SampledImage,
	ResourceType_StorageImage,
	ResourceType_Sampler,
	ResourceType_CombinedImageSampler,
	ResourceType_AccelerationStructure
};

VkDescriptorType ResourceTypeVkDescriptorType(const ResourceType type);

struct PassResources
{
	ResourceType m_type;
	int m_binding;
	int m_set;
	int m_resourceCount;

	VkBuffer m_buffer;
	VkImageView m_view;
	VkSampler m_sampler;
	VkAccelerationStructureKHR* m_acc;
	int m_offset = 0;
	int m_range = 0;

	static PassResources SetBufferResource(
		int binding, int set, int resourceCount,
		MCBV buffer, int offset, int range
	);

	static PassResources SetBufferResource(
		int binding, int set, int resourceCount,
		StorageBuffer buffer, int offset, int range
	);

	static PassResources SetSampledImageResource(
		int binding, int set, int resourceCount,
		VkImageView view
	);

	static PassResources SetStorageImageResource(
		int binding, int set, int resourceCount,
		VkImageView view
	);

	static PassResources SetSamplerResource(
		int binding, int set, int resourceCount,
		VkSampler sampler
	);

	static PassResources SetCombinedImageSamplerResource(
		int binding, int set, int resourceCount,
		VkImageView view, VkSampler sampler
	);

	static PassResources SetCombinedImageSamplerResource(
		int binding, int set, int resourceCount,
		VkAccelerationStructureKHR* accel
	);
};




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
	//int					m_setIndex;
	VkDevice			m_device;
	VkDescriptorPool	m_pool;
	VkDescriptorSet		m_set;
};

class MVulkanDescriptorSetWrite {
public:
	void Update(
		VkDevice device,
		VkDescriptorSet set,
		std::vector<std::vector<VkDescriptorBufferInfo>> constantBufferInfos,
		std::vector<std::vector<VkDescriptorBufferInfo>> storageBufferInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> combinedImageInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> seperateImageInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> storageImageInfos,
		std::vector<VkDescriptorImageInfo> seperateSamplers,
		std::vector<VkAccelerationStructureKHR> accelerationStructures
		);
		

	void Update(
		VkDevice device,
		VkDescriptorSet set,
		std::vector<VkDescriptorBufferInfo> bufferInfos,
		std::vector<std::vector<VkDescriptorImageInfo>> imageInfos);

	void Update(
		VkDevice device,
		VkDescriptorSet set,
		std::vector<PassResources> resources);

//private:
//	VkWriteDescriptorSet m_write;
};
#endif