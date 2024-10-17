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
};

class MVulkanBuffer {
public:
	MVulkanBuffer(BufferType _type);

	void Create(MVulkanDevice device, BufferCreateInfo info);
	void LoadData(VkDevice device, void* data);
	void Map(VkDevice device);
	void UnMap(VkDevice device);

	void Clean(VkDevice device);

	inline VkBuffer& GetBuffer() { return buffer; }
	inline VkDeviceMemory& GetBufferMemory() { return bufferMemory; }
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
	void UpdateData(MVulkanDevice device, void* data);
	inline VkBuffer& GetBuffer() { return dataBuffer.GetBuffer(); }
	inline VkDeviceMemory& GetBufferMemory() { return dataBuffer.GetBufferMemory(); }
private:
	MVulkanBuffer dataBuffer;
	//MVulkanBuffer stagingBuffer;
};

//class UBO :public MVulkanBuffer {
//public:
//	UBO();
//};

class VertexBuffer {
public:
	VertexBuffer();
	void Clean(VkDevice device);
	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data);
	inline VkBuffer& GetBuffer() { return dataBuffer.GetBuffer(); }
	inline VkDeviceMemory& GetBufferMemory() { return dataBuffer.GetBufferMemory(); }
private:
	MVulkanBuffer dataBuffer;
	MVulkanBuffer stagingBuffer;
};

class IndexBuffer {
public:
	IndexBuffer();
	void Clean(VkDevice device);
	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data);
	inline VkBuffer& GetBuffer() { return dataBuffer.GetBuffer(); }
	inline VkDeviceMemory& GetBufferMemory() { return dataBuffer.GetBufferMemory(); }
private:
	MVulkanBuffer dataBuffer;
	MVulkanBuffer stagingBuffer;
};

struct ImageCreateInfo {
	uint32_t width, height, channels, mipLevels=1;
	VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
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

	inline VkImage GetImage()const { return image; }
	inline VkDeviceMemory GetImageMemory() const { return imageMemory; }
	inline VkImageView GetImageView()const { return view; }

	inline uint32_t GetMipLevel()const {
		return mipLevel;
	};

private:
	uint32_t mipLevel = 1;
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView view;
};

class MVulkanTexture {
public:
public:
	MVulkanTexture();
	void Clean(VkDevice device);

	template<typename T>
	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo, MImage<T>* imageData)
	{
		image.CreateImage(device, imageInfo);
		image.CreateImageView(device, viewInfo);

		BufferCreateInfo binfo;
		binfo.size = imageInfo.width * imageInfo.height * 4;
		binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBuffer.Create(device, binfo);

		stagingBuffer.Map(device.GetDevice());
		stagingBuffer.LoadData(device.GetDevice(), imageData->GetData());
		stagingBuffer.UnMap(device.GetDevice());

		commandList->TransitionImageLayout(image.GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image.GetMipLevel());
		commandList->CopyBufferToImage(stagingBuffer.GetBuffer(), image.GetImage(), static_cast<uint32_t>(imageInfo.width), static_cast<uint32_t>(imageInfo.height));
		commandList->TransitionImageLayout(image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image.GetMipLevel());

		//stagingBuffer.Clean(device.GetDevice());
	}

	inline VkImage GetImage() { return image.GetImage(); }
	inline VkImageView GetImageView() { return image.GetImageView(); }
	inline VkDeviceMemory GetImageMemory() { return image.GetImageMemory(); }

private:
	uint32_t mipLevels;
	MVulkanImage image;
	MVulkanBuffer stagingBuffer;
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
