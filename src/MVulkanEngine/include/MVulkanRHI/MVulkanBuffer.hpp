#pragma once
#ifndef MVULKANBUFFER_HPP
#define MVULKANBUFFER_HPP

#include "vulkan/vulkan_core.h"
#include "MVulkanDevice.hpp"
#include "MVulkanCommand.hpp"
#include "Utils/MImage.hpp"

struct BufferCreateInfo {
	VkBufferUsageFlagBits usage;
	VkDeviceSize size;
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	uint32_t arrayLength = 1;
};

struct StorageImageCreateInfo {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;

	VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
};


class MVulkanBuffer {
public:
	MVulkanBuffer(BufferType _type);
	void Clean();

	void Create(MVulkanDevice device, BufferCreateInfo info);
	void LoadData(const void* data, uint32_t offset=0, uint32_t datasize=0);
	void Map();
	void UnMap();

	inline VkBuffer& GetBuffer() { return m_buffer; }
	inline VkDeviceMemory& GetBufferMemory() { return m_bufferMemory; }

	inline uint32_t GetBufferSize() const {
		return m_bufferSize;
	}

	inline const VkDeviceAddress& GetBufferAddress() const {return m_deviceAddress;}
protected:

	VkDevice			m_device;
	void*				m_mappedData;
	BufferType			m_type;
	VkDeviceSize		m_bufferSize;
	VkBuffer			m_buffer;
	VkBufferView		m_bufferView = nullptr;
	VkDeviceMemory		m_bufferMemory;

	VkDeviceAddress		m_deviceAddress;
};

class MSRV :public MVulkanBuffer {

};

class MUAV :public MVulkanBuffer {

};

class MCBV {
public:
	MCBV();
	//~MCBV();
	void Clean();

	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data);
	void Create(MVulkanDevice device, BufferCreateInfo info);
	void UpdateData(float offset=0, void* data=nullptr);
	inline VkBuffer& GetBuffer() { return m_dataBuffer.GetBuffer(); }
	inline VkDeviceMemory& GetBufferMemory() { return m_dataBuffer.GetBufferMemory(); }
	const inline uint32_t GetArrayLength() const { return m_info.arrayLength; }
	const inline uint32_t GetBufferSize() const { return m_info.size; }
private:

	BufferCreateInfo	m_info;
	MVulkanBuffer		m_dataBuffer;
};

class StorageBuffer {
public:
	StorageBuffer();
	//~MCBV();
	void Clean();

	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data);
	void Create(MVulkanDevice device, BufferCreateInfo info);
	void UpdateData(float offset = 0, void* data = nullptr);
	inline VkBuffer& GetBuffer() { return m_dataBuffer.GetBuffer(); }
	inline VkDeviceMemory& GetBufferMemory() { return m_dataBuffer.GetBufferMemory(); }
	const inline uint32_t GetArrayLength() const { return m_info.arrayLength; }
	const inline uint32_t GetBufferSize() const { return m_info.size; }
private:

	BufferCreateInfo	m_info;
	MVulkanBuffer		m_dataBuffer;
};

class Buffer {
public:
	Buffer(BufferType type = BufferType::NONE);
	void Clean();

	void Create(MVulkanDevice device, BufferCreateInfo info);
	void CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, const void* data);
	inline VkBuffer& GetBuffer() { return m_dataBuffer.GetBuffer(); }
	inline const VkDeviceAddress& GetBufferAddress() { return m_dataBuffer.GetBufferAddress(); }
	inline const VkDeviceMemory& GetBufferMemory() { return m_dataBuffer.GetBufferMemory(); }
private:

	BufferType		m_type;
	MVulkanBuffer	m_dataBuffer;
	MVulkanBuffer	m_stagingBuffer;
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
	MVulkanImage() = default;
	void Clean();

	void CreateImage(MVulkanDevice device, ImageCreateInfo info);
	void CreateImageView(ImageViewCreateInfo info);

	inline VkImage GetImage()const { return m_image; }
	inline VkDeviceMemory GetImageMemory() const { return m_imageMemory; }
	inline VkImageView GetImageView()const { return m_view; }

	inline uint32_t GetMipLevel()const {
		return m_mipLevel;
	};

	static MVulkanImage CreateSwapchainImage(VkImage image, VkImageView view) {
		MVulkanImage mImage;
		mImage.m_image = image;
		mImage.m_view = view;
		return mImage;
	}

private:
	VkDevice		m_device;
	uint32_t		m_mipLevel = 1;
	VkImage			m_image;
	VkDeviceMemory	m_imageMemory;
	VkImageView		m_view;
};

class MVulkanTexture {
public:
	MVulkanTexture();
	void Clean();

	template<typename T>
	void CreateAndLoadData(
		MVulkanCommandList* commandList, MVulkanDevice device, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo, std::vector<MImage<T>*> imageDatas, uint32_t mipLevel = 0)
	{
		m_imageInfo = imageInfo;
		m_viewInfo = viewInfo;
		m_imageValid = true;
	
		m_image.CreateImage(device, imageInfo);
		m_image.CreateImageView(viewInfo);
	
		MVulkanImageMemoryBarrier barrier{};
		barrier.image = m_image.GetImage();
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.levelCount = m_image.GetMipLevel();
		barrier.baseArrayLayer = 0;
		barrier.layerCount = imageDatas.size();
	
		commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		BufferCreateInfo binfo;
		binfo.size = imageDatas.size() * imageInfo.width * imageInfo.height * 4;
		binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		m_stagingBuffer.Create(device, binfo);
		m_stagingBufferUsed = true;
	
		for (auto layer = 0; layer < imageDatas.size(); layer++) {
		
			uint32_t offset = layer * imageInfo.width * imageInfo.height * 4;
			m_stagingBuffer.Map();
			m_stagingBuffer.LoadData(imageDatas[layer]->GetData(), offset, imageInfo.width * imageInfo.height * 4);
			m_stagingBuffer.UnMap();
	
			commandList->CopyBufferToImage(m_stagingBuffer.GetBuffer(), m_image.GetImage(), static_cast<uint32_t>(imageInfo.width), static_cast<uint32_t>(imageInfo.height), offset, mipLevel, uint32_t(layer));
			
		}
	
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	void Create(MVulkanCommandList* commandList, MVulkanDevice device, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo, VkImageLayout layout);

	inline VkImage GetImage() const { return m_image.GetImage(); }
	inline VkImageView GetImageView() const { return m_image.GetImageView(); }
	inline VkDeviceMemory GetImageMemory() const { return m_image.GetImageMemory(); }

	inline VkFormat GetFormat() const { return m_imageInfo.format; }
	inline ImageCreateInfo GetImageInfo() const { return m_imageInfo; }
private:
	ImageCreateInfo		m_imageInfo;
	ImageViewCreateInfo m_viewInfo;

	uint32_t			m_mipLevels = 1;

	bool				m_imageValid = false;
	MVulkanImage		m_image;
	bool				m_stagingBufferUsed = false;
	MVulkanBuffer		m_stagingBuffer;
};

//class IndirectBuffer {
//public:
//	IndirectBuffer() = default;
//
//	void Create(MVulkanDevice device, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,int32_t  vertexOffset, uint32_t firstInstance);
//	void Clean();
//private:
//	VkDrawIndexedIndirectCommand	m_drawCommand;
//	MVulkanBuffer					m_indirectBuffer;
//};

//class StorageBuffer
//{
//public:
//	StorageBuffer() = default;
//
//	void Create(MVulkanDevice device, uint32_t size);
//	void LoadData(void* data, uint32_t size, uint32_t offset=0);
//	void Clean();
//
//	inline const VkDeviceAddress& GetAddress(){return m_storageBuffer.GetBufferAddress();}
//
//private:
//	MVulkanBuffer					m_storageBuffer;
//	MVulkanBuffer					m_stagingBuffer;
//};


#endif
