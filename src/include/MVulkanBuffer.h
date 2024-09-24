#pragma once
#ifndef MVULKANBUFFER_H
#define MVULKANBUFFER_H

#include "vulkan/vulkan_core.h"
#include "MVulkanDevice.h"
#include "MVulkanCommand.h"

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

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

	void Clean(VkDevice device);

	inline VkBuffer& GetBuffer() { return buffer; }
	inline VkDeviceMemory& GetBufferMemory() { return bufferMemory; }
protected:
	BufferType type;
	VkDeviceSize bufferSize;
	VkBuffer buffer;
	VkBufferView bufferView;
	VkDeviceMemory bufferMemory;
};

class SRV :public MVulkanBuffer {

};

class UAV :public MVulkanBuffer {

};

class CBV :public MVulkanBuffer {

};

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

#endif
