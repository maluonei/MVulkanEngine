#include "MVulkanRHI/MVulkanBuffer.hpp"
#include <stdexcept>
#include <MVulkanRHI/MVulkanEngine.hpp>
#include <spdlog/spdlog.h>

MVulkanBuffer::MVulkanBuffer(BufferType _type):m_type(_type)
{

}


void MVulkanBuffer::Create(MVulkanDevice device, BufferCreateInfo info)
{
    m_active = true;
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
    m_active = false;
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

void MVulkanTexture::MLoadImage(
    MVulkanCommandList* commandList, 
    MVulkanCommandQueue commandQueue, 
    MVulkanDevice device, 
    fs::path imagePath)
{
    auto extension = imagePath.extension().string();
    if (extension == ".dds") {
        gli::texture2d tex(gli::load(imagePath.string().c_str()));//) = gli::texture2d(gli::load(imagePath.string().c_str()));
        //gli::texture tex = gli::load(imagePath.string().c_str());
        
        if (tex.empty()) {
            spdlog::error("Failed to load DDS file: {0}", imagePath.string().c_str());
        }

        gli::format gliFormat = tex.format();
        VkFormat vkFormat = ConvertGliFormatToVkFormat(gliFormat);
        if (vkFormat == VK_FORMAT_UNDEFINED) {
            //throw std::runtime_error("Unsupported format");
            spdlog::error("unknown VkFormat when creating texture from dds file");
        }

        const glm::tvec3<uint32_t>& extent = tex[0].extent();
        //const glm::tvec3<uint32_t>& extent = tex.extent();
        uint32_t mipLevels = static_cast<uint32_t>(tex.levels());
        //mipLevels = 1;

        ImageCreateInfo imageInfo{};
        imageInfo.width = extent.x;
        imageInfo.height = extent.y;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.format = vkFormat;
        imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.mipLevels = mipLevels;

        ImageViewCreateInfo viewInfo{};
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.format = vkFormat;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        //viewInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = mipLevels;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;

        this->Create(device, imageInfo, viewInfo);

        std::vector<void*> datas(mipLevels);
        std::vector<uint32_t> sizes(mipLevels);
        std::vector<VkOffset3D> origins(mipLevels);
        std::vector<VkExtent3D> extents(mipLevels);
        
        for (auto mip = 0; mip < mipLevels; mip++) {
            datas[mip] = tex[mip].data();
            sizes[mip] = tex[mip].size();
            origins[mip] = { 0, 0, 0 };
            extents[mip] = { static_cast<uint32_t>(tex[mip].extent().x), static_cast<uint32_t>(tex[mip].extent().y), 1 };
        }
        //for (auto mip = 0; mip < mipLevels; mip++) {
        //    datas[mip] = tex.data();
        //    sizes[mip] = tex.size();
        //    origins[mip] = { 0, 0, 0 };
        //    extents[mip] = { static_cast<uint32_t>(tex.extent().x), static_cast<uint32_t>(tex.extent().y), 1 };
        //}
        
        commandList->GetFence().WaitForSignal();
        commandList->GetFence().Reset();
        commandList->Reset();
        commandList->Begin();
        LoadData(commandList, device, mipLevels, datas, sizes, origins, extents);
        commandList->End();
        
        Singleton<MVulkanEngine>::instance().SubmitCommands(*commandList, commandQueue);
        commandQueue.WaitForQueueComplete();
        m_stagingBuffer.Clean();

        m_mipMapGenerated = true;
    }
    else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
        //LoadImageFile(commandList, device, imagePath);
        MImage<unsigned char> image;
        if (image.Load(imagePath)) {
            int mipLevels = image.MipLevels();
            //int mipLevels = 1;

            ImageCreateInfo imageInfo{};
            imageInfo.width = image.Width();
            imageInfo.height = image.Height();
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            imageInfo.format = image.Format();
            imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.mipLevels = mipLevels;

            ImageViewCreateInfo viewInfo{};
            viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.format = image.Format();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            //viewInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.baseMipLevel = 0;
            viewInfo.levelCount = mipLevels;
            viewInfo.baseArrayLayer = 0;
            viewInfo.layerCount = 1;
            viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;

            commandList->GetFence().WaitForSignal();
            commandList->GetFence().Reset();
            commandList->Reset();
            commandList->Begin();
            std::vector<MImage<unsigned char>*> images(1, &image);
            CreateAndLoadData(commandList, device, imageInfo, viewInfo, images);
            commandList->End();

            Singleton<MVulkanEngine>::instance().SubmitCommands(*commandList, commandQueue);
            commandQueue.WaitForQueueComplete();
            m_stagingBuffer.Clean();
        }
        else {
            spdlog::error("fail to load image: {0}", imagePath.string());
        }
    }
    else {
        throw std::runtime_error("Unsupported image format");
    }

    ImageCreateInfo imageInfo;
    ImageViewCreateInfo viewInfo;
}

void MVulkanTexture::LoadData(
    MVulkanCommandList* commandList, 
    MVulkanDevice device, 
    void* data, 
    uint32_t size, 
    uint32_t offset)
{
    MVulkanImageMemoryBarrier barrier{};
    barrier.image = m_image.GetImage();
    //barrier.srcAccessMask = 0;
    barrier.srcAccessMask = TextureState2AccessFlag(m_state.m_state);
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = TextureState2ImageLayout(m_state.m_state);
    //barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.levelCount = m_image.GetMipLevel();
    barrier.baseArrayLayer = 0;
    barrier.layerCount = 1;

    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    BufferCreateInfo binfo;
    binfo.size = size;
    binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    if (m_stagingBuffer.IsActive()) {
        m_stagingBuffer.Clean();
    }
    m_stagingBuffer.Create(device, binfo);
    m_stagingBufferUsed = true;

    //for (auto layer = 0; layer < imageDatas.size(); layer++) {

    //uint32_t offset = 0;
    m_stagingBuffer.Map();
    m_stagingBuffer.LoadData(data, offset, size);
    m_stagingBuffer.UnMap();

    commandList->CopyBufferToImage(m_stagingBuffer.GetBuffer(), m_image.GetImage(), static_cast<uint32_t>(m_imageInfo.width), static_cast<uint32_t>(m_imageInfo.height), static_cast<uint32_t>(m_imageInfo.depth), offset, 0, uint32_t(0));

    //}

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    m_state.m_state = ETextureState::SRV;
    m_state.m_stage = ShaderStageFlagBits::FRAGMENT;
}

void MVulkanTexture::LoadData(MVulkanCommandList* commandList, MVulkanDevice device, int mip, void* data, uint32_t size, uint32_t offset)
{
    MVulkanImageMemoryBarrier barrier{};
    barrier.image = m_image.GetImage();
    barrier.srcAccessMask = TextureState2AccessFlag(m_state.m_state);
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = TextureState2ImageLayout(m_state.m_state);
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.baseArrayLayer = 0;
    barrier.layerCount = 1;
    barrier.baseMipLevel = mip;
    barrier.levelCount = 1;

    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    BufferCreateInfo binfo;
    binfo.size = size;
    binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    if (m_stagingBuffer.IsActive()) {
        m_stagingBuffer.Clean();
    }
    m_stagingBuffer.Create(device, binfo);
    m_stagingBufferUsed = true;

    //for (auto layer = 0; layer < imageDatas.size(); layer++) {

    //uint32_t offset = 0;
    m_stagingBuffer.Map();
    m_stagingBuffer.LoadData(data, offset, size);
    m_stagingBuffer.UnMap();

    commandList->CopyBufferToImage(
        m_stagingBuffer.GetBuffer(), 
        m_image.GetImage(), 
        static_cast<uint32_t>(m_imageInfo.width), 
        static_cast<uint32_t>(m_imageInfo.height),
        static_cast<uint32_t>(m_imageInfo.depth), 
        offset, 
        0, 
        uint32_t(0));

    //}

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    m_state.m_state = ETextureState::SRV;
    m_state.m_stage = ShaderStageFlagBits::FRAGMENT;
}

void MVulkanTexture::LoadData(
    MVulkanCommandList* commandList,
    MVulkanDevice device, 
    int mip, 
    void* data,
    uint32_t size,
    uint32_t offset, 
    VkOffset3D origin,
    VkExtent3D extent)
{
    //todo 传入每一层的stage

    MVulkanImageMemoryBarrier barrier{};
    barrier.image = m_image.GetImage();
    barrier.srcAccessMask = TextureState2AccessFlag(m_state.m_state);
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = TextureState2ImageLayout(m_state.m_state);
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.baseArrayLayer = 0;
    barrier.layerCount = 1;
    barrier.baseMipLevel = mip;
    barrier.levelCount = 1;

    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    BufferCreateInfo binfo;
    binfo.size = size;
    binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    if (!m_stagingBuffer.IsActive()) {
        m_stagingBuffer.Create(device, binfo);
        m_stagingBufferUsed = true;
    }

    m_stagingBuffer.Map();
    m_stagingBuffer.LoadData(data, offset, size);
    m_stagingBuffer.UnMap();

    commandList->CopyBufferToImage(
        m_stagingBuffer.GetBuffer(), m_image.GetImage(),
        origin, extent,
        offset, mip, uint32_t(0));

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    m_state.m_state = ETextureState::SRV;
    m_state.m_stage = ShaderStageFlagBits::FRAGMENT;
}

void MVulkanTexture::LoadData(MVulkanCommandList* commandList, MVulkanDevice device, void* data, uint32_t size, uint32_t offset, VkOffset3D origin, VkExtent3D extent)
{
    MVulkanImageMemoryBarrier barrier{};
    barrier.image = m_image.GetImage();
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.levelCount = m_image.GetMipLevel();
    barrier.baseArrayLayer = 0;
    barrier.layerCount = 1;

    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    BufferCreateInfo binfo;
    binfo.size = size;
    binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    if (!m_stagingBuffer.IsActive()) {
        m_stagingBuffer.Create(device, binfo);
        m_stagingBufferUsed = true;
    }

    //for (auto layer = 0; layer < imageDatas.size(); layer++) {

    //uint32_t offset = 0;
    m_stagingBuffer.Map();
    m_stagingBuffer.LoadData(data, offset, size);
    m_stagingBuffer.UnMap();

    commandList->CopyBufferToImage(
        m_stagingBuffer.GetBuffer(), m_image.GetImage(), 
        origin, extent,
        offset, 0, uint32_t(0));

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    m_state.m_state = ETextureState::GENERAL;
    //m_state.m_stage = ShaderStageFlagBits::FRAGMENT;
}

void MVulkanTexture::LoadData(
    MVulkanCommandList* commandList, 
    MVulkanDevice device, 
    int mips, 
    std::vector<void*> datas,
    std::vector<uint32_t> sizes,
    std::vector<VkOffset3D> origins,
    std::vector<VkExtent3D> extents)
{
    //todo 传入每一层的stage
    if (mips != sizes.size()) {
        spdlog::error("mips != sizes.size()");
    }

    MVulkanImageMemoryBarrier barrier{};
    barrier.image = m_image.GetImage();
    barrier.srcAccessMask = TextureState2AccessFlag(m_state.m_state);
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = TextureState2ImageLayout(m_state.m_state);
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.baseArrayLayer = 0;
    barrier.layerCount = 1;
    barrier.baseMipLevel = 0;
    barrier.levelCount = mips;

    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    
    uint32_t totalSize = 0;
    for (auto i = 0; i < mips; i++) {
        totalSize += sizes[i];
    }
    
    BufferCreateInfo binfo;
    binfo.size = totalSize;
    binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    if (!m_stagingBuffer.IsActive()) {
        m_stagingBuffer.Create(device, binfo);
        m_stagingBufferUsed = true;
    }
    
    m_stagingBuffer.Map();
    uint32_t totalOffset = 0;
    for (auto i = 0; i < mips; i++) {
        m_stagingBuffer.LoadData(datas[i], totalOffset, sizes[i]);

        commandList->CopyBufferToImage(
            m_stagingBuffer.GetBuffer(), m_image.GetImage(),
            origins[i], extents[i],
            totalOffset, i, uint32_t(0));

        totalOffset += sizes[i];
    }
    m_stagingBuffer.UnMap();

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    commandList->TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    m_state.m_state = ETextureState::SRV;
    m_state.m_stage = ShaderStageFlagBits::FRAGMENT;
}


void MVulkanTexture::TransferTextureState(int currentFrame, TextureState newState)
{
    if (m_state.m_state == newState.m_state && m_state.m_stage == newState.m_stage)
        return;

    ChangeImageLayout(currentFrame, m_state, newState);
    m_state = newState;
}

void MVulkanTexture::TransferTextureState(MVulkanCommandList commandList, TextureState newState)
{
    if (m_state.m_state == newState.m_state && m_state.m_stage == newState.m_stage)
        return;

    ChangeImageLayout(commandList, m_state, newState);
    m_state = newState;
}

bool MVulkanTexture::TransferTextureStateGetBarrier(
    TextureState newState,
    MVulkanImageMemoryBarrier& barrier,
    VkPipelineStageFlags& srcStage,
    VkPipelineStageFlags& dstStage)
{
    if (m_state.m_state == newState.m_state && m_state.m_stage == newState.m_stage)
        return false;

    barrier = ChangeImageLayout(m_state, newState, srcStage, dstStage);
    m_state = newState;
    return true;
}


const bool MVulkanTexture::MipMapGenerated() const
{
    return m_mipMapGenerated;
}

void MVulkanTexture::ChangeImageLayout(int currentFrame, TextureState oldState, TextureState newState)
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
    //_barrier.baseMipLevel = GetImageInfo().mipLevels - 1;
    _barrier.baseMipLevel = 0;
    _barrier.levelCount = GetImageInfo().mipLevels;
    _barrier.aspectMask = m_viewInfo.flag;
    barriers.push_back(_barrier);

    if (currentFrame < 0) {
        spdlog::error("invalid currentFrame value:{0}", currentFrame);
        //Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, srcStage, dstStage);
    }
    else {
        Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, srcStage, dstStage);
    }
}

void MVulkanTexture::ChangeImageLayout(
    MVulkanCommandList commandList,
    TextureState oldState,
    TextureState newState)
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
    _barrier.baseMipLevel = 0;
    _barrier.levelCount = GetImageInfo().mipLevels;
    //_barrier.levelCount = 1;
    _barrier.aspectMask = m_viewInfo.flag;
    barriers.push_back(_barrier);

    Singleton<MVulkanEngine>::instance().TransitionImageLayout2(commandList, barriers, srcStage, dstStage);
}

MVulkanImageMemoryBarrier MVulkanTexture::ChangeImageLayout(
    TextureState oldState, 
    TextureState newState,
    VkPipelineStageFlags& srcStage,
    VkPipelineStageFlags& dstStage)
{
    MVulkanImageMemoryBarrier _barrier{};
    //VkPipelineStageFlags srcStage, dstStage;

    srcStage = TextureState2PipelineStage(oldState);
    dstStage = TextureState2PipelineStage(newState);

    _barrier.image = m_image.GetImage();
    _barrier.srcAccessMask = TextureState2AccessFlag(oldState.m_state);
    _barrier.dstAccessMask = TextureState2AccessFlag(newState.m_state);
    _barrier.oldLayout = TextureState2ImageLayout(oldState.m_state);
    _barrier.newLayout = TextureState2ImageLayout(newState.m_state);
    _barrier.baseMipLevel = 0;
    _barrier.levelCount = GetImageInfo().mipLevels;
    //_barrier.levelCount = 1;
    _barrier.aspectMask = m_viewInfo.flag;

    return _barrier;
}

VkFormat ConvertGliFormatToVkFormat(gli::format format)
{
    switch (format) {
    case gli::FORMAT_UNDEFINED:
        return VK_FORMAT_UNDEFINED;

        // 8-bit formats
    case gli::FORMAT_R8_UNORM_PACK8:
        return VK_FORMAT_R8_UNORM;
    case gli::FORMAT_R8_SNORM_PACK8:
        return VK_FORMAT_R8_SNORM;
    case gli::FORMAT_R8_UINT_PACK8:
        return VK_FORMAT_R8_UINT;
    case gli::FORMAT_R8_SINT_PACK8:
        return VK_FORMAT_R8_SINT;
    case gli::FORMAT_R8_SRGB_PACK8:
        return VK_FORMAT_R8_SRGB;

    case gli::FORMAT_RG8_UNORM_PACK8:
        return VK_FORMAT_R8G8_UNORM;
    case gli::FORMAT_RG8_SNORM_PACK8:
        return VK_FORMAT_R8G8_SNORM;
    case gli::FORMAT_RG8_UINT_PACK8:
        return VK_FORMAT_R8G8_UINT;
    case gli::FORMAT_RG8_SINT_PACK8:
        return VK_FORMAT_R8G8_SINT;
    case gli::FORMAT_RG8_SRGB_PACK8:
        return VK_FORMAT_R8G8_SRGB;

    case gli::FORMAT_RGB8_UNORM_PACK8:
        return VK_FORMAT_R8G8B8_UNORM;
    case gli::FORMAT_RGB8_SNORM_PACK8:
        return VK_FORMAT_R8G8B8_SNORM;
    case gli::FORMAT_RGB8_UINT_PACK8:
        return VK_FORMAT_R8G8B8_UINT;
    case gli::FORMAT_RGB8_SINT_PACK8:
        return VK_FORMAT_R8G8B8_SINT;
    case gli::FORMAT_RGB8_SRGB_PACK8:
        return VK_FORMAT_R8G8B8_SRGB;

    case gli::FORMAT_RGBA8_UNORM_PACK8:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case gli::FORMAT_RGBA8_SNORM_PACK8:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case gli::FORMAT_RGBA8_UINT_PACK8:
        return VK_FORMAT_R8G8B8A8_UINT;
    case gli::FORMAT_RGBA8_SINT_PACK8:
        return VK_FORMAT_R8G8B8A8_SINT;
    case gli::FORMAT_RGBA8_SRGB_PACK8:
        return VK_FORMAT_R8G8B8A8_SRGB;

        // 16-bit formats
    case gli::FORMAT_R16_UNORM_PACK16:
        return VK_FORMAT_R16_UNORM;
    case gli::FORMAT_R16_SNORM_PACK16:
        return VK_FORMAT_R16_SNORM;
    case gli::FORMAT_R16_UINT_PACK16:
        return VK_FORMAT_R16_UINT;
    case gli::FORMAT_R16_SINT_PACK16:
        return VK_FORMAT_R16_SINT;
    case gli::FORMAT_R16_SFLOAT_PACK16:
        return VK_FORMAT_R16_SFLOAT;

    case gli::FORMAT_RG16_UNORM_PACK16:
        return VK_FORMAT_R16G16_UNORM;
    case gli::FORMAT_RG16_SNORM_PACK16:
        return VK_FORMAT_R16G16_SNORM;
    case gli::FORMAT_RG16_UINT_PACK16:
        return VK_FORMAT_R16G16_UINT;
    case gli::FORMAT_RG16_SINT_PACK16:
        return VK_FORMAT_R16G16_SINT;
    case gli::FORMAT_RG16_SFLOAT_PACK16:
        return VK_FORMAT_R16G16_SFLOAT;

    case gli::FORMAT_RGB16_UNORM_PACK16:
        return VK_FORMAT_R16G16B16_UNORM;
    case gli::FORMAT_RGB16_SNORM_PACK16:
        return VK_FORMAT_R16G16B16_SNORM;
    case gli::FORMAT_RGB16_UINT_PACK16:
        return VK_FORMAT_R16G16B16_UINT;
    case gli::FORMAT_RGB16_SINT_PACK16:
        return VK_FORMAT_R16G16B16_SINT;
    case gli::FORMAT_RGB16_SFLOAT_PACK16:
        return VK_FORMAT_R16G16B16_SFLOAT;

    case gli::FORMAT_RGBA16_UNORM_PACK16:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case gli::FORMAT_RGBA16_SNORM_PACK16:
        return VK_FORMAT_R16G16B16A16_SNORM;
    case gli::FORMAT_RGBA16_UINT_PACK16:
        return VK_FORMAT_R16G16B16A16_UINT;
    case gli::FORMAT_RGBA16_SINT_PACK16:
        return VK_FORMAT_R16G16B16A16_SINT;
    case gli::FORMAT_RGBA16_SFLOAT_PACK16:
        return VK_FORMAT_R16G16B16A16_SFLOAT;

        // 32-bit formats
    case gli::FORMAT_R32_UINT_PACK32:
        return VK_FORMAT_R32_UINT;
    case gli::FORMAT_R32_SINT_PACK32:
        return VK_FORMAT_R32_SINT;
    case gli::FORMAT_R32_SFLOAT_PACK32:
        return VK_FORMAT_R32_SFLOAT;

    case gli::FORMAT_RG32_UINT_PACK32:
        return VK_FORMAT_R32G32_UINT;
    case gli::FORMAT_RG32_SINT_PACK32:
        return VK_FORMAT_R32G32_SINT;
    case gli::FORMAT_RG32_SFLOAT_PACK32:
        return VK_FORMAT_R32G32_SFLOAT;

    case gli::FORMAT_RGB32_UINT_PACK32:
        return VK_FORMAT_R32G32B32_UINT;
    case gli::FORMAT_RGB32_SINT_PACK32:
        return VK_FORMAT_R32G32B32_SINT;
    case gli::FORMAT_RGB32_SFLOAT_PACK32:
        return VK_FORMAT_R32G32B32_SFLOAT;

    case gli::FORMAT_RGBA32_UINT_PACK32:
        return VK_FORMAT_R32G32B32A32_UINT;
    case gli::FORMAT_RGBA32_SINT_PACK32:
        return VK_FORMAT_R32G32B32A32_SINT;
    case gli::FORMAT_RGBA32_SFLOAT_PACK32:
        return VK_FORMAT_R32G32B32A32_SFLOAT;

        // Compressed formats
    case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8:
        return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8:
        return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
    case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16:
        return VK_FORMAT_BC2_UNORM_BLOCK;
    case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16:
        return VK_FORMAT_BC2_SRGB_BLOCK;
    case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16:
        return VK_FORMAT_BC3_UNORM_BLOCK;
    case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16:
        return VK_FORMAT_BC3_SRGB_BLOCK;

        // Depth-stencil formats
    case gli::FORMAT_D16_UNORM_PACK16:
        return VK_FORMAT_D16_UNORM;
    case gli::FORMAT_D24_UNORM_PACK32:
        return VK_FORMAT_X8_D24_UNORM_PACK32;
    case gli::FORMAT_D32_SFLOAT_PACK32:
        return VK_FORMAT_D32_SFLOAT;
    case gli::FORMAT_S8_UINT_PACK8:
        return VK_FORMAT_S8_UINT;
    case gli::FORMAT_D16_UNORM_S8_UINT_PACK32:
        return VK_FORMAT_D16_UNORM_S8_UINT;
    case gli::FORMAT_D24_UNORM_S8_UINT_PACK32:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case gli::FORMAT_D32_SFLOAT_S8_UINT_PACK64:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;

    case gli::FORMAT_RGB_BP_UFLOAT_BLOCK16:
        return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16:
        return VK_FORMAT_BC6H_SFLOAT_BLOCK;
    case gli::FORMAT_RGBA_BP_UNORM_BLOCK16:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    case gli::FORMAT_RGBA_BP_SRGB_BLOCK16:
        return VK_FORMAT_BC7_SRGB_BLOCK;

        // Add additional mappings as needed

    default:
        return VK_FORMAT_UNDEFINED;
    }
}