#include "MVulkanBuffer.h"
#include <stdexcept>

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

MVulkanBuffer::MVulkanBuffer(BufferType _type):type(_type)
{

}

void MVulkanBuffer::Create(MVulkanDevice device, BufferCreateInfo info)
{
    bufferSize = info.size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = info.size;
    bufferInfo.usage = info.usage;
    bufferInfo.sharingMode = info.sharingMode;

    if (vkCreateBuffer(device.GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.GetDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(device.GetPhysicalDevice(), memRequirements.memoryTypeBits, BufferType2VkMemoryPropertyFlags(type));

    if (vkAllocateMemory(device.GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device.GetDevice(), buffer, bufferMemory, 0);
}

void MVulkanBuffer::LoadData(VkDevice device, void* data)
{
    void* _data;
    vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &_data);
    memcpy(data, data, bufferSize);
    vkUnmapMemory(device, bufferMemory);
}

void MVulkanBuffer::Clean(VkDevice device)
{

}

ShaderInputBuffer::ShaderInputBuffer():dataBuffer(BufferType::SHADER_INPUT), stagingBuffer(BufferType::STAGING)
{

}

void ShaderInputBuffer::CreateAndLoadData(MVulkanCommandList commandList, MVulkanDevice device, BufferCreateInfo info, void* data)
{
    info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dataBuffer.Create(device, info);

    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.Create(device, info);

    stagingBuffer.LoadData(device.GetDevice(), data);

    commandList.CopyBuffer(stagingBuffer.GetBuffer(), dataBuffer.GetBuffer(), info.size);
}
