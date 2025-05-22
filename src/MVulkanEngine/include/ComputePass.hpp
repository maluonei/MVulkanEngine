#pragma once
#ifndef COMPUTEPASS_HPP
#define COMPUTEPASS_HPP

#include "MVulkanRHI/MVulkanPipeline.hpp"
#include "MVulkanRHI/MVulkanRenderPass.hpp"
#include "MVulkanRHI/MVulkanFrameBuffer.hpp"
#include "MVulkanRHI/MVulkanDevice.hpp"
#include "MVulkanRHI/MVulkanDescriptor.hpp"
#include "MVulkanRHI/MVulkanCommand.hpp"
#include "MVulkanRHI/MVulkanSwapchain.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanDescriptorWrite.hpp"
#include "Utils/VulkanUtil.hpp"

class ComputeShaderModule;

class ComputePass {
public:
	ComputePass(MVulkanDevice device);
	void Clean();

	void Create(std::shared_ptr<ComputeShaderModule> shader,
		MVulkanDescriptorSetAllocator& allocator);
	void UpdateDescriptorSetWrite(std::vector<PassResources> resources);
	void CreatePipeline(
		MVulkanDescriptorSetAllocator& allocator);


	//void RecreateStorageImages(std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos);
	std::shared_ptr<ComputeShaderModule> GetShader() const { return m_shader; }
	inline MVulkanComputePipeline GetPipeline()const { return m_pipeline; }
	//inline MVulkanDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }
	//inline MVulkanDescriptorSetLayouts GetDescriptorSetLayout() const { return m_descriptorLayout; }
	MVulkanDescriptorSetLayouts GetDescriptorSetLayout(int set) const;// { return m_descriptorLayout; }
	std::unordered_map<int, MVulkanDescriptorSet> GetDescriptorSets() const;// { return m_descriptorSet; }

	StorageBuffer GetStorageBufferByBinding(uint32_t binding);
	VkImageView GetStorageImageViewByBinding(uint32_t binding, uint32_t arrayIndex);

	void LoadConstantBuffer(uint32_t alignment);
	void LoadStorageBuffer(uint32_t alignment);

	//void PrepareResourcesForShaderRead();
	void PrepareResourcesForShaderRead(MVulkanCommandList commandList);

private:
	void setShader(std::shared_ptr<ComputeShaderModule> shader);


	MVulkanDevice					m_device;
	MVulkanComputePipeline			m_pipeline;

	//MVulkanDescriptorSetLayouts		m_descriptorLayout;
	std::vector<MVulkanDescriptorSetLayouts> m_descriptorLayouts;
	std::unordered_map<int, MVulkanDescriptorSet> m_descriptorSets;

	//MVulkanDescriptorSet			m_descriptorSet;

	std::shared_ptr<ComputeShaderModule>	m_shader;

	//std::vector<MVulkanDescriptorSetLayoutBinding> m_bindings;

	uint32_t m_cbvCount = 0;
	uint32_t m_storageBufferCount = 0;
	uint32_t m_separateSamplerCount = 0;
	uint32_t m_separateImageCount = 0;
	uint32_t m_storageImageCount = 0;
	uint32_t m_combinedImageCount = 0;

	std::vector<MCBV>				m_constantBuffer;
	std::vector<StorageBuffer>		m_storageBuffer;
	std::vector<std::vector<std::shared_ptr<MVulkanTexture>>> m_storageImages;

	void updateResourceCache(ShaderResourceKey key, const PassResources& resources);
	std::unordered_map<ShaderResourceKey, MVulkanDescriptorSetLayoutBinding> m_bindings;
	std::unordered_map<ShaderResourceKey, PassResources> m_cachedResources;
};

#endif

