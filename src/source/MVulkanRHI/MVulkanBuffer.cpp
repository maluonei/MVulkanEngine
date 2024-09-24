#include "MVulkanRHI/MVulkanBuffer.h"
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

    VK_CHECK_RESULT(vkCreateBuffer(device.GetDevice(), &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.GetDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(device.GetPhysicalDevice(), memRequirements.memoryTypeBits, BufferType2VkMemoryPropertyFlags(type));

    VK_CHECK_RESULT(vkAllocateMemory(device.GetDevice(), &allocInfo, nullptr, &bufferMemory));

    vkBindBufferMemory(device.GetDevice(), buffer, bufferMemory, 0);
}

void MVulkanBuffer::LoadData(VkDevice device, void* data)
{
    void* _data;
    vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &_data);
    memcpy(_data, data, bufferSize);
    vkUnmapMemory(device, bufferMemory);
}

void MVulkanBuffer::Clean(VkDevice device)
{
    
}

VertexBuffer::VertexBuffer():dataBuffer(BufferType::SHADER_INPUT), stagingBuffer(BufferType::STAGING)
{

}

void VertexBuffer::Clean(VkDevice device)
{
    dataBuffer.Clean(device);
}

void VertexBuffer::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data)
{
    info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dataBuffer.Create(device, info);

    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.Create(device, info);

    stagingBuffer.LoadData(device.GetDevice(), data);

    commandList->CopyBuffer(stagingBuffer.GetBuffer(), dataBuffer.GetBuffer(), info.size);

    stagingBuffer.Clean(device.GetDevice());
}

IndexBuffer::IndexBuffer() :dataBuffer(BufferType::SHADER_INPUT), stagingBuffer(BufferType::STAGING)
{

}

void IndexBuffer::Clean(VkDevice device)
{
    dataBuffer.Clean(device);
}

void IndexBuffer::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data)
{
    info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    dataBuffer.Create(device, info);

    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.Create(device, info);

    stagingBuffer.LoadData(device.GetDevice(), data);

    commandList->CopyBuffer(stagingBuffer.GetBuffer(), dataBuffer.GetBuffer(), info.size);

    stagingBuffer.Clean(device.GetDevice());
}