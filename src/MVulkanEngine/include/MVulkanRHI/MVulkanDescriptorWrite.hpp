#pragma once
#ifndef MVULKANDESCRIPTORWRITE_HPP
#define MVULKANDESCRIPTORWRITE_HPP

#include "vulkan/vulkan_core.h"
#include <vector>
#include <variant>
#include "Utils/VulkanUtil.hpp"
#include "MVulkanBuffer.hpp"


//VkDescriptorType ResourceTypeVkDescriptorType(const ResourceType type);
//class StorageBuffer;
//class MVulkanTexture;

struct PassResources
{
	ResourceType m_type;
	int m_binding;
	int m_set;
	//int m_resourceCount;

	std::vector<VkBuffer> m_buffers;
	//std::vector<VkImageView> m_views;
	//std::vector<VkImageView> m_views;
	std::vector<TextureSubResource> m_textures;
	std::vector<VkSampler> m_samplers;
	std::vector<VkAccelerationStructureKHR*> m_accs;
	int m_offset = 0;
	int m_range = 0;

	static PassResources SetBufferResource(
		int binding, int set, 
		MCBV buffer, int offset, int range
	);

	static PassResources SetBufferResource(
		std::string name, int binding, int set
	);

	static PassResources SetBufferResource(
		std::string name, int binding, int set, int frameIndex
	);

	static PassResources SetBufferResource(
		std::string name, std::string shaderResourceName, 
		int binding, int set, int frameIndex
	);

	static PassResources SetBufferResource(
		int binding, int set,
		std::shared_ptr<StorageBuffer> buffer
	);

	static PassResources SetSampledImageResource(
		int binding, int set,
		std::shared_ptr<MVulkanTexture> texture
	);

	static PassResources SetSampledImageResource(
		int binding, int set,
		TextureSubResource texture
	);

	static PassResources SetSampledImageResource(
		int binding, int set, std::vector<TextureSubResource> textures
	);

	static PassResources SetSampledImageResource(
		int binding, int set, std::vector<std::shared_ptr<MVulkanTexture>> textures
	);

	static PassResources SetStorageImageResource(
		int binding, int set,
		std::shared_ptr<MVulkanTexture> texture
	);

	static PassResources SetStorageImageResource(
		int binding, int set,
		TextureSubResource texture
	);

	static PassResources SetStorageImageResource(
		int binding, int set,
		std::vector<TextureSubResource> textures
	);

	static PassResources SetSamplerResource(
		int binding, int set,
		VkSampler sampler
	);

	static PassResources SetCombinedImageSamplerResource(
		int binding, int set,
		TextureSubResource texture, VkSampler sampler
	);

	static PassResources SetAccelerationStructureResource(
		int binding, int set,
		VkAccelerationStructureKHR* accel
	);
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
};

#endif

