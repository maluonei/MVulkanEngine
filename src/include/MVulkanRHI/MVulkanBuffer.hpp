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
	uint32_t width, height, channels;
	VkFormat format;
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlags properties;
};

class MVulkanImage {
public:
	void Create(MVulkanDevice device, ImageCreateInfo info);
	void Clean(VkDevice deivce);

	inline VkImage GetImage()const { return image; }
	inline VkDeviceMemory GetImageMemory() const { return imageMemory; }
	inline VkImageView GetImageView()const { return view; }

private:
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
	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, ImageCreateInfo info, MImage<T>* imageData)
	{
		info.usage = VkBufferUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		info.format = imageData->Format();
		info.width = imageData->Width();
		info.height = imageData->Height();
		info.channels = imageData->Channels();
		image.Create(device, info);

		BufferCreateInfo binfo;
		binfo.size = info.width * info.height * 4;
		binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBuffer.Create(device, binfo);

		stagingBuffer.Map(device.GetDevice());
		stagingBuffer.LoadData(device.GetDevice(), imageData->GetData());
		stagingBuffer.UnMap(device.GetDevice());

		commandList->TransitionImageLayout(image.GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		commandList->CopyBufferToImage(stagingBuffer.GetBuffer(), image.GetImage(), static_cast<uint32_t>(info.width), static_cast<uint32_t>(info.height));
		commandList->TransitionImageLayout(image.GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//stagingBuffer.Clean(device.GetDevice());
	}

	inline VkImage GetImage() { return image.GetImage(); }
	inline VkImageView GetImageView() { return image.GetImageView(); }
	inline VkDeviceMemory GetImageMemory() { return image.GetImageMemory(); }

private:
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
