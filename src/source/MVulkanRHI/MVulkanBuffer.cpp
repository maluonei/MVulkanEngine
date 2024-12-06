#include "MVulkanRHI/MVulkanBuffer.hpp"
#include <stdexcept>
#include <MVulkanRHI/MVulkanEngine.hpp>

//uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
//    VkPhysicalDeviceMemoryProperties memProperties;
//    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
//
//    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
//        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
//            return i;
//        }
//    }
//
//    throw std::runtime_error("failed to find suitable memory type!");
//}

MVulkanBuffer::MVulkanBuffer(BufferType _type):type(_type)
{

}

void MVulkanBuffer::Create(MVulkanDevice device, BufferCreateInfo info)
{
    bufferSize = info.size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = CalculateAlignedSize(info.size, Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment()) * info.arrayLength;
    bufferInfo.usage = info.usage;
    bufferInfo.sharingMode = info.sharingMode;

    VK_CHECK_RESULT(vkCreateBuffer(device.GetDevice(), &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.GetDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, BufferType2VkMemoryPropertyFlags(type));

    VK_CHECK_RESULT(vkAllocateMemory(device.GetDevice(), &allocInfo, nullptr, &bufferMemory));

    vkBindBufferMemory(device.GetDevice(), buffer, bufferMemory, 0);

    //vkMapMemory(device.GetDevice(), bufferMemory, 0, bufferSize, 0, &mappedData);
    //Map(device.GetDevice());
}

void MVulkanBuffer::LoadData(VkDevice device, const void* data, uint32_t offset)
{
    memcpy((char*)mappedData + offset, data, bufferSize);
}

void MVulkanBuffer::Map(VkDevice device)
{
    vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &mappedData);
}

void MVulkanBuffer::UnMap(VkDevice device)
{
    vkUnmapMemory(device, bufferMemory);
}

void MVulkanBuffer::Clean(VkDevice device)
{
    //UnMap(device);
    vkDestroyBuffer(device, buffer, nullptr);
    //vkDestroyBufferView(device, bufferView, nullptr);
}

//VertexBuffer::VertexBuffer():dataBuffer(BufferType::SHADER_INPUT), stagingBuffer(BufferType::STAGING)
//{
//
//}
//
//void VertexBuffer::Clean(VkDevice device)
//{
//    dataBuffer.Clean(device);
//}
//
//void VertexBuffer::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, const void* data)
//{
//    info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
//    dataBuffer.Create(device, info);
//
//    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
//    stagingBuffer.Create(device, info);
//
//    stagingBuffer.Map(device.GetDevice());
//    stagingBuffer.LoadData(device.GetDevice(), data);
//    stagingBuffer.UnMap(device.GetDevice());
//
//    commandList->CopyBuffer(stagingBuffer.GetBuffer(), dataBuffer.GetBuffer(), info.size);
//
//    stagingBuffer.Clean(device.GetDevice());
//}
//
//IndexBuffer::IndexBuffer() :dataBuffer(BufferType::SHADER_INPUT), stagingBuffer(BufferType::STAGING)
//{
//
//}
//
//void IndexBuffer::Clean(VkDevice device)
//{
//    dataBuffer.Clean(device);
//}
//
//void IndexBuffer::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, const void* data)
//{
//    info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
//    dataBuffer.Create(device, info);
//
//    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
//    stagingBuffer.Create(device, info);
//
//    stagingBuffer.Map(device.GetDevice());
//    stagingBuffer.LoadData(device.GetDevice(), data);
//    stagingBuffer.UnMap(device.GetDevice());
//
//    commandList->CopyBuffer(stagingBuffer.GetBuffer(), dataBuffer.GetBuffer(), info.size);
//
//    stagingBuffer.Clean(device.GetDevice());
//}

Buffer::Buffer(BufferType type) :dataBuffer(BufferType::SHADER_INPUT), stagingBuffer(BufferType::STAGING_BUFFER), m_type(type)
{

}

void Buffer::Clean(VkDevice device)
{
    dataBuffer.Clean(device);
}

void Buffer::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, const void* data)
{
    //info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    info.usage = BufferType2VkBufferUsageFlagBits(m_type);
    dataBuffer.Create(device, info);

    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.Create(device, info);

    stagingBuffer.Map(device.GetDevice());
    stagingBuffer.LoadData(device.GetDevice(), data);
    stagingBuffer.UnMap(device.GetDevice());

    commandList->CopyBuffer(stagingBuffer.GetBuffer(), dataBuffer.GetBuffer(), info.size);

    stagingBuffer.Clean(device.GetDevice());
}

MCBV::MCBV():dataBuffer(BufferType::CBV)
{

}

void MCBV::Clean(VkDevice device)
{
    dataBuffer.UnMap(device);
    dataBuffer.Clean(device);
}

void MCBV::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data)
{
    info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    dataBuffer.Create(device, info);

    dataBuffer.Map(device.GetDevice());
    dataBuffer.LoadData(device.GetDevice(), data);

}

void MCBV::Create( MVulkanDevice device, BufferCreateInfo info)
{
    info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    m_info = info;

    dataBuffer.Create(device, info);

    dataBuffer.Map(device.GetDevice());
}

void MCBV::UpdateData(MVulkanDevice device, float offset, void* data)
{
    dataBuffer.LoadData(device.GetDevice(), data, offset);
}

void MVulkanImage::CreateImage(MVulkanDevice device, ImageCreateInfo info)
{
    // 创建 Image
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = info.width;
    imageInfo.extent.height = info.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = info.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = info.format; //显存中数据存储格式
    imageInfo.tiling = info.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = info.usage;
    imageInfo.samples = info.samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // 设置 image 格式、尺寸等
    VK_CHECK_RESULT(vkCreateImage(device.GetDevice(), &imageInfo, nullptr, &m_image));

    mipLevel = info.mipLevels;

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.GetDevice(), m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, info.properties);

    VK_CHECK_RESULT(vkAllocateMemory(device.GetDevice(), &allocInfo, nullptr, &m_imageMemory));

    vkBindImageMemory(device.GetDevice(), m_image, m_imageMemory, 0);
}

void MVulkanImage::CreateImageView(MVulkanDevice device, ImageViewCreateInfo info)
{
    // 创建 ImageView
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = info.viewType;
    viewInfo.format = info.format; //gpu中数据访问格式
    viewInfo.subresourceRange.aspectMask = info.flag;
    viewInfo.subresourceRange.baseMipLevel = info.baseMipLevel;
    viewInfo.subresourceRange.levelCount = info.levelCount;
    viewInfo.subresourceRange.baseArrayLayer = info.baseArrayLayer;
    viewInfo.subresourceRange.layerCount = info.layerCount;

    VK_CHECK_RESULT(vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &m_view));
}

void MVulkanImage::Clean(VkDevice device)
{
    vkDestroyImageView(device, m_view, nullptr);
    vkDestroyImage(device, m_image, nullptr);
    vkFreeMemory(device, m_imageMemory, nullptr);
}

MVulkanTexture::MVulkanTexture():image(), stagingBuffer(BufferType::STAGING_BUFFER)
{
}

void MVulkanTexture::Clean(VkDevice device)
{
}

void IndirectBuffer::Create(MVulkanDevice device, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    drawCommand.indexCount = indexCount;
    drawCommand.instanceCount = instanceCount;
    drawCommand.firstIndex = firstIndex;
    drawCommand.vertexOffset = vertexOffset;
    drawCommand.firstInstance = firstInstance;

    BufferCreateInfo info;
    info.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    info.size = sizeof(drawCommand);

    indirectBuffer.Create(device, info);

    indirectBuffer.Map(device.GetDevice());
    indirectBuffer.LoadData(device.GetDevice(), &drawCommand);
    indirectBuffer.UnMap(device.GetDevice());
}
