#pragma once
#ifndef COMPUTEPASS_HPP
#define COMPUTEPASS_HPP

#include "MVulkanRHI/MVulkanPipeline.hpp"
#include "Shaders/ShaderModule.hpp"

struct StorageBufferCreateInfo {

};

class ComputePass {
public:
	ComputePass(MVulkanDevice device);
	void Create(std::shared_ptr<ComputeShaderModule> shader, MVulkanDescriptorSetAllocator& allocator, 
		std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos,
		std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers);
	void CreatePipeline(MVulkanDescriptorSetAllocator& allocator, 
		std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos,
		std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers);

	void UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers);

	std::shared_ptr<ComputeShaderModule> GetShader() const { return m_shader; }
	inline MVulkanComputePipeline GetPipeline()const { return m_pipeline; }
	inline MVulkanDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }
	inline MVulkanDescriptorSetLayouts GetDescriptorSetLayout() const { return m_descriptorLayout; }

	StorageBuffer GetStorageBufferByBinding(uint32_t binding);

	//void LoadConstantAndStorageBuffer(uint32_t alignment);
	void LoadConstantBuffer(uint32_t alignment);
	void LoadStorageBuffer(uint32_t alignment);

private:
	void setShader(std::shared_ptr<ComputeShaderModule> shader);


	MVulkanDevice					m_device;
	MVulkanComputePipeline			m_pipeline;

	MVulkanDescriptorSetLayouts		m_descriptorLayout;
	MVulkanDescriptorSet			m_descriptorSet;

	std::shared_ptr<ComputeShaderModule>	m_shader;

	uint32_t m_cbvCount = 0;
	uint32_t m_storageBufferCount = 0;
	uint32_t m_separateSamplerCount = 0;
	uint32_t m_separateImageCount = 0;
	uint32_t m_storageImageCount = 0;
	uint32_t m_combinedImageCount = 0;

	std::vector<MCBV>				m_constantBuffer;
	std::vector<StorageBuffer>		m_storageBuffer;
	std::vector<std::vector<std::shared_ptr<MVulkanTexture>>> m_storageImages;
};

#endif
