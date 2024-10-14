#include "MVulkanRHI/MVulkanBuffer.hpp"
#include <stdexcept>

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
    bufferInfo.size = info.size;
    bufferInfo.usage = info.usage;
    bufferInfo.sharingMode = info.sharingMode;

    VK_CHECK_RESULT(vkCreateBuffer(device.GetDevice(), &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.GetDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, BufferType2VkMemoryPropertyFlags(type));

    VK_CHECK_RESULT(vkAllocateMemory(device.GetDevice(), &allocInfo, nullptr, &bufferMemory));

    vkBindBufferMemory(device.GetDevice(), buffer, bufferMemory, 0);

    //vkMapMemory(device.GetDevice(), bufferMemory, 0, bufferSize, 0, &mappedData);
    //Map(device.GetDevice());
}

void MVulkanBuffer::LoadData(VkDevice device, void* data)
{
    //vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &mappedData);
    memcpy(mappedData, data, bufferSize);
    //vkUnmapMemory(device, bufferMemory);
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

    stagingBuffer.Map(device.GetDevice());
    stagingBuffer.LoadData(device.GetDevice(), data);
    stagingBuffer.UnMap(device.GetDevice());

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

    dataBuffer.Create(device, info);

    dataBuffer.Map(device.GetDevice());
}

void MCBV::UpdateData(MVulkanDevice device, void* data)
{
    dataBuffer.LoadData(device.GetDevice(), data);
}

void MVulkanImage::Create(MVulkanDevice device, ImageCreateInfo info)
{
    //VkImageFormatProperties formatProperties;
    //VkResult result = vkGetPhysicalDeviceImageFormatProperties(
    //    device.GetPhysicalDevice(),
    //    VK_FORMAT_R8G8B8_UINT,
    //    VK_IMAGE_TYPE_2D,
    //    VK_IMAGE_TILING_OPTIMAL,
    //    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //    0,  // flags
    //    &formatProperties
    //);
    //
    //if (result != VK_SUCCESS) {
    //    // 该格式或配置不被支持
    //    std::cerr << "Format not supported!" << std::endl;
    //}

    // 创建 Image
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = info.width;
    imageInfo.extent.height = info.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = info.format; //显存中数据存储格式
    imageInfo.tiling = info.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = info.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // 设置 image 格式、尺寸等
    VK_CHECK_RESULT(vkCreateImage(device.GetDevice(), &imageInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.GetDevice(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, info.properties);

    VK_CHECK_RESULT(vkAllocateMemory(device.GetDevice(), &allocInfo, nullptr, &imageMemory));

    vkBindImageMemory(device.GetDevice(), image, imageMemory, 0);


    // 创建 ImageView
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = info.format; //gpu中数据访问格式
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK_RESULT(vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &view));
}

void MVulkanImage::Clean(VkDevice deivce)
{
}

MVulkanTexture::MVulkanTexture():image(), stagingBuffer(BufferType::STAGING)
{
}

void MVulkanTexture::Clean(VkDevice device)
{
}