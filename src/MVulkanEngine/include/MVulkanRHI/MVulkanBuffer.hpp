#pragma once
#ifndef MVULKANBUFFER_H
#define MVULKANBUFFER_H

#include "vulkan/vulkan_core.h"
#include "MVulkanDevice.hpp"
#include "MVulkanCommand.hpp"
#include "Utils/MImage.hpp"
//uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

struct BufferCreateInfo {
	VkBufferUsageFlagBits usage;
	VkDeviceSize size;
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	uint32_t arrayLength = 1;
};

class MVulkanBuffer {
public:
	MVulkanBuffer(BufferType _type);

	void Create(MVulkanDevice device, BufferCreateInfo info);
	void LoadData(VkDevice device, const void* data, uint32_t offset=0, uint32_t datasize=0);
	void Map(VkDevice device);
	void UnMap(VkDevice device);

	void Clean(VkDevice device);

	inline VkBuffer& GetBuffer() { return buffer; }
	inline VkDeviceMemory& GetBufferMemory() { return bufferMemory; }

	inline uint32_t GetBufferSize() const {
		return bufferSize;
	}
protected:
	void* mappedData;
	BufferType type;
	VkDeviceSize bufferSize;
	VkBuffer buffer;
	VkBufferView bufferView;
	VkDeviceMemory bufferMemory;
};

class MSRV :public MVulkanBuffer {

};

class MUAV :public MVulkanBuffer {

};

class MCBV {
public:
	MCBV();
	void Clean(VkDevice device);
	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data);
	void Create(MVulkanDevice device, BufferCreateInfo info);
	void UpdateData(MVulkanDevice device, float offset=0, void* data=nullptr);
	inline VkBuffer& GetBuffer() { return dataBuffer.GetBuffer(); }
	inline VkDeviceMemory& GetBufferMemory() { return dataBuffer.GetBufferMemory(); }
	const inline uint32_t GetArrayLength() const { return m_info.arrayLength; }
	const inline uint32_t GetBufferSize() const { return m_info.size; }
private:
	BufferCreateInfo m_info;
	MVulkanBuffer dataBuffer;
	//MVulkanBuffer stagingBuffer;
};

//class UBO :public MVulkanBuffer {
//public:
//	UBO();
//};

//class VertexBuffer {
//public:
//	VertexBuffer();
//	void Clean(VkDevice device);
//	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, const void* data);
//	inline VkBuffer& GetBuffer() { return dataBuffer.GetBuffer(); }
//	inline VkDeviceMemory& GetBufferMemory() { return dataBuffer.GetBufferMemory(); }
//private:
//	MVulkanBuffer dataBuffer;
//	MVulkanBuffer stagingBuffer;
//};
//
//class IndexBuffer {
//public:
//	IndexBuffer();
//	void Clean(VkDevice device);
//	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, const void* data);
//	inline VkBuffer& GetBuffer() { return dataBuffer.GetBuffer(); }
//	inline VkDeviceMemory& GetBufferMemory() { return dataBuffer.GetBufferMemory(); }
//private:
//	MVulkanBuffer dataBuffer;
//	MVulkanBuffer stagingBuffer;
//};

class Buffer {
public:
	Buffer(BufferType type = BufferType::NONE);
	void Clean(VkDevice device);
	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, const void* data);
	inline VkBuffer& GetBuffer() { return dataBuffer.GetBuffer(); }
	inline VkDeviceMemory& GetBufferMemory() { return dataBuffer.GetBufferMemory(); }
private:
	BufferType m_type;
	MVulkanBuffer dataBuffer;
	MVulkanBuffer stagingBuffer;
};

struct ImageCreateInfo {
	uint32_t width, height, channels, mipLevels=1;
	uint32_t arrayLength = 1;

	VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkImageCreateFlags flag = 0;
};

struct ImageViewCreateInfo {
	VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	VkImageAspectFlagBits flag = VK_IMAGE_ASPECT_COLOR_BIT;
	uint32_t baseMipLevel = 0;
	uint32_t levelCount = 1;

	uint32_t baseArrayLayer = 0;
	uint32_t layerCount = 1;
	VkComponentMapping components;
	//VkImageSubresourceRange subresourceRange;
};

class MVulkanImage {
public:
	void CreateImage(MVulkanDevice device, ImageCreateInfo info);
	void CreateImageView(MVulkanDevice device, ImageViewCreateInfo info);
	void Clean(VkDevice deivce);

	inline VkImage GetImage()const { return m_image; }
	inline VkDeviceMemory GetImageMemory() const { return m_imageMemory; }
	inline VkImageView GetImageView()const { return m_view; }

	inline uint32_t GetMipLevel()const {
		return mipLevel;
	};

	static MVulkanImage CreateSwapchainImage(VkImage image, VkImageView view) {
		MVulkanImage mImage;
		mImage.m_image = image;
		mImage.m_view = view;
		return mImage;
	}

private:
	uint32_t mipLevel = 1;
	VkImage m_image;
	VkDeviceMemory m_imageMemory;
	VkImageView m_view;
};

class MVulkanTexture {
public:
	MVulkanTexture();
	void Clean(VkDevice device);

	template<typename T>
	void CreateAndLoadData(
		MVulkanCommandList* commandList, MVulkanDevice device, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo, std::vector<MImage<T>*> imageDatas, uint32_t mipLevel = 0)
	{
		this->imageInfo = imageInfo;
		this->viewInfo = viewInfo;

		image.CreateImage(device, imageInfo);
		image.CreateImageView(device, viewInfo);

		MVulkanImageMemoryBarrier barrier{};
		barrier.image = image.GetImage();
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.levelCount = image.GetMipLevel();
		barrier.baseArrayLayer = 0;
		barrier.layerCount = imageDatas.size();

		commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		BufferCreateInfo binfo;
		binfo.size = imageDatas.size() * imageInfo.width * imageInfo.height * 4;
		binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBuffer.Create(device, binfo);

		for (auto layer = 0; layer < imageDatas.size(); layer++) {
		
			uint32_t offset = layer * imageInfo.width * imageInfo.height * 4;
			stagingBuffer.Map(device.GetDevice());
			stagingBuffer.LoadData(device.GetDevice(), imageDatas[layer]->GetData(), offset, imageInfo.width * imageInfo.height * 4);
			stagingBuffer.UnMap(device.GetDevice());

			commandList->CopyBufferToImage(stagingBuffer.GetBuffer(), image.GetImage(), static_cast<uint32_t>(imageInfo.width), static_cast<uint32_t>(imageInfo.height), offset, mipLevel, uint32_t(layer));
			
		}

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	void Create(MVulkanCommandList* commandList, MVulkanDevice device, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo, VkImageLayout layout);

	inline VkImage GetImage() const { return image.GetImage(); }
	inline VkImageView GetImageView() const { return image.GetImageView(); }
	inline VkDeviceMemory GetImageMemory() const { return image.GetImageMemory(); }

	inline VkFormat GetFormat() const { return imageInfo.format; }
	inline ImageCreateInfo GetImageInfo() const { return imageInfo; }
private:
	ImageCreateInfo imageInfo;
	ImageViewCreateInfo viewInfo;

	uint32_t mipLevels;
	MVulkanImage image;
	MVulkanBuffer stagingBuffer;
};

class IndirectBuffer {
public:

	void Create(MVulkanDevice device, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,int32_t  vertexOffset, uint32_t firstInstance);

private:
	VkDrawIndexedIndirectCommand drawCommand;
	MVulkanBuffer indirectBuffer;
};

//class MVulkanSampler {
//public:
//	void Create(MVulkanDevice device);
//	void Clean(VkDevice deivce);
//
//private:
//	VkSampler sampler;
//
//};

#endif
