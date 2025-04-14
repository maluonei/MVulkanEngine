#include "MVulkanRHI/MVulkanBuffer.hpp"
#include <stdexcept>
#include <MVulkanRHI/MVulkanEngine.hpp>

MVulkanBuffer::MVulkanBuffer(BufferType _type):m_type(_type)
{

}


void MVulkanBuffer::Create(MVulkanDevice device, BufferCreateInfo info)
{
    m_info = info;
    m_device = device.GetDevice();
    m_bufferSize = info.size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = CalculateAlignedSize(info.size, Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment()) * info.arrayLength;
    bufferInfo.usage = info.usage;
    bufferInfo.sharingMode = info.sharingMode;

    VK_CHECK_RESULT(vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, BufferType2VkMemoryPropertyFlags(m_type));
    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
    if (info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT; // 设置此标志
        memoryAllocateFlagsInfo.pNext = nullptr;
        allocInfo.pNext = &memoryAllocateFlagsInfo;
    }

    VK_CHECK_RESULT(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_bufferMemory));

    vkBindBufferMemory(device.GetDevice(), m_buffer, m_bufferMemory, 0);

    //only storage buffer need device address
    if (info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo deviceAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        deviceAddressInfo.buffer = m_buffer;
        m_deviceAddress = vkGetBufferDeviceAddress(m_device, &deviceAddressInfo);
    }
}

void MVulkanBuffer::LoadData(const void* data, uint32_t offset, uint32_t datasize)
{
    memcpy((char*)m_mappedData + offset, data, datasize);
}

void MVulkanBuffer::Map()
{
    vkMapMemory(m_device, m_bufferMemory, 0, m_bufferSize, 0, &m_mappedData);
}

void MVulkanBuffer::UnMap()
{
    vkUnmapMemory(m_device, m_bufferMemory);
}

void MVulkanBuffer::Clean()
{
    vkFreeMemory(m_device, m_bufferMemory, nullptr);
    vkDestroyBuffer(m_device, m_buffer, nullptr);
    if(m_bufferView)
        vkDestroyBufferView(m_device, m_bufferView, nullptr);
}

Buffer::Buffer(BufferType type) :m_dataBuffer(BufferType::SHADER_INPUT), m_stagingBuffer(BufferType::STAGING_BUFFER), m_type(type)
{

}

void Buffer::Clean()
{
    m_stagingBuffer.Clean();
    m_dataBuffer.Clean();
}

void Buffer::Create(MVulkanDevice device, BufferCreateInfo info)
{
    info.usage = BufferType2VkBufferUsageFlagBits(m_type);
    m_dataBuffer.Create(device, info);

    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    m_stagingBuffer.Create(device, info);

    //m_stagingBuffer.Map();
    //m_stagingBuffer.LoadData(data, 0, info.size);
    //m_stagingBuffer.UnMap();
//
    //commandList->CopyBuffer(m_stagingBuffer.GetBuffer(), m_dataBuffer.GetBuffer(), info.size);
}

void Buffer::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, const void* data)
{
    info.usage = BufferType2VkBufferUsageFlagBits(m_type);
    m_dataBuffer.Create(device, info);

    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    m_stagingBuffer.Create(device, info);

    m_stagingBuffer.Map();
    m_stagingBuffer.LoadData(data, 0, info.size);
    m_stagingBuffer.UnMap();

    commandList->CopyBuffer(m_stagingBuffer.GetBuffer(), m_dataBuffer.GetBuffer(), info.size);
}

MCBV::MCBV():m_dataBuffer(BufferType::CBV)
{

}

void MCBV::Clean()
{
    m_dataBuffer.UnMap();
    m_dataBuffer.Clean();
}

void MCBV::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data)
{
    Create(device, info);
    m_dataBuffer.LoadData(data, 0, info.size);

}

void MCBV::Create( MVulkanDevice device, BufferCreateInfo info)
{
    info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    m_info = info;

    m_dataBuffer.Create(device, info);

    m_dataBuffer.Map();
}

void MCBV::UpdateData(float offset, void* data)
{
    m_dataBuffer.LoadData(data, offset, m_dataBuffer.GetBufferSize());
}


StorageBuffer::StorageBuffer(VkBufferUsageFlagBits usage) :m_dataBuffer(BufferType::STORAGE_BUFFER), m_usage(usage)
{
}

void StorageBuffer::Clean()
{
    m_dataBuffer.UnMap();
    m_dataBuffer.Clean();
}

void StorageBuffer::CreateAndLoadData(MVulkanCommandList* commandList, MVulkanDevice device, BufferCreateInfo info, void* data)
{
    Create(device, info);
    m_dataBuffer.LoadData(data, 0, info.size);
}

void StorageBuffer::Create(MVulkanDevice device, BufferCreateInfo info)
{
    info.usage = VkBufferUsageFlagBits(m_usage);
    m_info = info;

    m_dataBuffer.Create(device, info);

    m_dataBuffer.Map();
}

void StorageBuffer::UpdateData(float offset, void* data)
{
    m_dataBuffer.LoadData(data, offset, m_dataBuffer.GetBufferSize());
}

void MVulkanImage::CreateImage(MVulkanDevice device, ImageCreateInfo info)
{
    m_device = device.GetDevice();

    // ���� Image
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = info.type;
    imageInfo.extent.width = info.width;
    imageInfo.extent.height = info.height;
    imageInfo.extent.depth = info.depth;
    imageInfo.mipLevels = info.mipLevels;
    imageInfo.arrayLayers = info.arrayLength;
    imageInfo.format = info.format; //�Դ������ݴ洢��ʽ
    imageInfo.tiling = info.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = info.usage;
    imageInfo.samples = info.samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = info.flag;
    // ���� image ��ʽ���ߴ��
    VK_CHECK_RESULT(vkCreateImage(device.GetDevice(), &imageInfo, nullptr, &m_image));
    //spdlog::info("create image:{0}", static_cast<void*>(m_image));

    m_mipLevel = info.mipLevels;

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.GetDevice(), m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, info.properties);

    VK_CHECK_RESULT(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory));

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);
}

void MVulkanImage::CreateImageView(ImageViewCreateInfo info)
{
    // ���� ImageView
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = info.viewType;
    viewInfo.format = info.format; //gpu�����ݷ��ʸ�ʽ
    viewInfo.subresourceRange.aspectMask = info.flag;
    viewInfo.subresourceRange.baseMipLevel = info.baseMipLevel;
    viewInfo.subresourceRange.levelCount = info.levelCount;
    viewInfo.subresourceRange.baseArrayLayer = info.baseArrayLayer;
    viewInfo.subresourceRange.layerCount = info.layerCount;

    VK_CHECK_RESULT(vkCreateImageView(m_device, &viewInfo, nullptr, &m_view));
}

void MVulkanImage::Clean()
{
    vkFreeMemory(m_device, m_imageMemory, nullptr);
    vkDestroyImageView(m_device, m_view, nullptr);
    vkDestroyImage(m_device, m_image, nullptr);
}

MVulkanTexture::MVulkanTexture():m_image(), m_stagingBuffer(BufferType::STAGING_BUFFER)
{
}


void MVulkanTexture::Clean()
{
    if (m_stagingBufferUsed)
        m_stagingBuffer.Clean();

    if(m_imageValid)
        m_image.Clean();
}

void MVulkanTexture::Create(
    MVulkanCommandList* commandList, 
    MVulkanDevice device, 
    ImageCreateInfo imageInfo, 
    ImageViewCreateInfo viewInfo, 
    VkImageLayout layout)
{
    m_imageInfo = imageInfo;
    m_viewInfo = viewInfo;
    m_imageValid = true;

    m_image.CreateImage(device, m_imageInfo);
    m_image.CreateImageView(m_viewInfo);

    MVulkanImageMemoryBarrier barrier{};
    barrier.image = m_image.GetImage();
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = layout;
    barrier.levelCount = m_image.GetMipLevel();
    barrier.baseArrayLayer = 0;
    barrier.layerCount = imageInfo.arrayLength;
    if (IsDepthFormat(imageInfo.format)) {
        barrier.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    //if(imageInfo.format == )

    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    //m_state.m_state = ImageLayout2TextureState(layout);
}

void MVulkanTexture::Create(MVulkanDevice device, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo)
{
    m_imageInfo = imageInfo;
    m_viewInfo = viewInfo;
    m_imageValid = true;

    m_image.CreateImage(device, m_imageInfo);
    m_image.CreateImageView(m_viewInfo);

    m_state.m_state = ETextureState::Undefined;
}

void MVulkanTexture::TransferTextureState(int imageIndex, TextureState newState)
{
    if (m_state.m_state == newState.m_state && m_state.m_stage == newState.m_stage)
        return;

    ChangeImageLayout(imageIndex, m_state, newState);
    m_state = newState;
}

void MVulkanTexture::ChangeImageLayout(int imageIndex, TextureState oldState, TextureState newState)
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    MVulkanImageMemoryBarrier _barrier{};
    VkPipelineStageFlags srcStage, dstStage;

    srcStage = TextureState2PipelineStage(oldState);
    dstStage = TextureState2PipelineStage(newState);

    _barrier.image = m_image.GetImage();
    _barrier.srcAccessMask = TextureState2AccessFlag(oldState.m_state);
    _barrier.dstAccessMask = TextureState2AccessFlag(newState.m_state);
    _barrier.oldLayout = TextureState2ImageLayout(oldState.m_state);
    _barrier.newLayout = TextureState2ImageLayout(newState.m_state);    
    _barrier.baseMipLevel = GetImageInfo().mipLevels - 1;
    barriers.push_back(_barrier);

    if (imageIndex < 0) {
        Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, srcStage, dstStage);
    }
    else {
        Singleton<MVulkanEngine>::instance().TransitionImageLayout2(imageIndex, barriers, srcStage, dstStage);
    }
}

