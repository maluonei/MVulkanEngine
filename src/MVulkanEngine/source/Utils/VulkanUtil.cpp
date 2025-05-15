#include "Utils/VulkanUtil.hpp"

VkDescriptorType ResourceType2VkDescriptorType(const ResourceType type)
{
    switch (type)
    {
    case ResourceType_ConstantBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case ResourceType_StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case ResourceType_SampledImage:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case ResourceType_StorageImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case ResourceType_CombinedImageSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case ResourceType_Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case ResourceType_AccelerationStructure:
        return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    default:
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

VkRenderingInfo RenderingInfo2VkRenderingInfo(CONST RenderingInfo& info) {
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    VkRenderingAttachmentInfo depthAttachments;

    for (auto i = 0; i < info.colorAttachments.size(); i++) {
        auto attachment = info.colorAttachments[i];

        VkRenderingAttachmentInfo ainfo;
        ainfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        ainfo.imageView = attachment.view;
        ainfo.imageLayout = attachment.layout;
        ainfo.loadOp = attachment.loadOp;
        ainfo.storeOp = attachment.storeOp;
        ainfo.clearValue.color = { {attachment.clearColor.x, attachment.clearColor.y, attachment.clearColor.z, attachment.clearColor.w} };

        colorAttachments.push_back(ainfo);
    }

    {
        auto depthAttachment = info.depthAttachment;

        depthAttachments.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachments.imageView = depthAttachment.view;
        depthAttachments.imageLayout = depthAttachment.layout;
        depthAttachments.loadOp = depthAttachment.loadOp;
        depthAttachments.storeOp = depthAttachment.storeOp;
        depthAttachments.clearValue.color = { {depthAttachment.clearColor.x, depthAttachment.clearColor.y, depthAttachment.clearColor.z, depthAttachment.clearColor.w} };
    }

    VkRenderingInfo renderingInfo;
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = info.offset;
    renderingInfo.renderArea.extent = info.extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = colorAttachments.size();
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = &depthAttachments;

    return renderingInfo;
}

bool IsDepthFormat(VkFormat format)
{
    if (
        format == VK_FORMAT_D16_UNORM ||
        format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
        format == VK_FORMAT_D32_SFLOAT ||
        format == VK_FORMAT_D16_UNORM_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT
        )
        return true;

    return false;
}

VkImageLayout TextureState2ImageLayout(const ETextureState state)
{
    switch (state) {

        case ETextureState::ColorAttachment:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ETextureState::DepthAttachment:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ETextureState::SRV:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ETextureState::UAV:
        case ETextureState::GENERAL:
            return VK_IMAGE_LAYOUT_GENERAL;
        case ETextureState::PRESENT:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case ETextureState::TRANSFER_SRC:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ETextureState::TRANSFER_DST:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ETextureState::Undefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VkAccessFlagBits TextureState2AccessFlag(const ETextureState state)
{
    VkAccessFlagBits flag = VkAccessFlagBits(0);
    //if (uint32_t(state) & uint32_t(ETextureState::GENERAL))
    //    flag = VkAccessFlagBits(flag | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    if(uint32_t(state) & uint32_t(ETextureState::ColorAttachment))
        flag = VkAccessFlagBits(flag | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    if(uint32_t(state) & uint32_t(ETextureState::DepthAttachment))
        flag = VkAccessFlagBits(flag | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    if(uint32_t(state) & uint32_t(ETextureState::SRV))
        flag = VkAccessFlagBits(flag | VK_ACCESS_SHADER_READ_BIT);
    if(uint32_t(state) & uint32_t(ETextureState::UAV))
        flag = VkAccessFlagBits(flag | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);
    if(uint32_t(state) & uint32_t(ETextureState::PRESENT))
        flag = VkAccessFlagBits(flag | VK_ACCESS_NONE);
    if (uint32_t(state) & uint32_t(ETextureState::TRANSFER_SRC))
        flag = VkAccessFlagBits(flag | VK_ACCESS_TRANSFER_READ_BIT);
    if (uint32_t(state) & uint32_t(ETextureState::TRANSFER_DST))
        flag = VkAccessFlagBits(flag | VK_ACCESS_TRANSFER_WRITE_BIT);
        
    return flag;

    //switch (state) {
    //case ETextureState::ColorAttachment:
    //    return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //case ETextureState::DepthAttachment:
    //    return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    //case ETextureState::SRV:
    //    return VK_ACCESS_SHADER_READ_BIT;
    //case ETextureState::UAV:
    //    return VkAccessFlagBits(VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);
    //case ETextureState::PRESENT:
    //    return VK_ACCESS_NONE;
    //case ETextureState::Undefined:
    //    return VK_ACCESS_NONE;
    //}
}

ShaderStageFlagBits VkShaderStage2ShaderStage(const VkShaderStageFlags stage)
{
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return ShaderStageFlagBits::VERTEX;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return ShaderStageFlagBits::FRAGMENT;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return ShaderStageFlagBits::GEOMETRY;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return ShaderStageFlagBits::COMPUTE;
        case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
            return ShaderStageFlagBits::RAYGEN;
        case VK_SHADER_STAGE_MISS_BIT_KHR:
            return ShaderStageFlagBits::MISS;
        case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
            return ShaderStageFlagBits::ANYHIT;
        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            return ShaderStageFlagBits::CLOESTHIT;
        case VK_SHADER_STAGE_TASK_BIT_EXT:
            return ShaderStageFlagBits::TASK;
        case VK_SHADER_STAGE_MESH_BIT_EXT:
            return ShaderStageFlagBits::MESH;
        //default:
        //    return ShaderStageFlagBits::NONE;
    }
}

//ETextureState ImageLayout2TextureState(const VkImageLayout layout)
//{
//    //switch(layout)
//}

VkPipelineStageFlags TextureState2PipelineStage(const TextureState state)
{
    VkPipelineStageFlags flag = 0;
    if(uint32_t(state.m_state) & uint32_t(ETextureState::ColorAttachment))
        flag = flag | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    if(uint32_t(state.m_state) & uint32_t(ETextureState::DepthAttachment))
        flag = flag | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    if(uint32_t(state.m_state) & uint32_t(ETextureState::SRV) || uint32_t(state.m_state) & uint32_t(ETextureState::UAV)){
        if(state.m_stage == ShaderStageFlagBits::VERTEX)
            flag = flag | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        if(state.m_stage == ShaderStageFlagBits::FRAGMENT)
            flag = flag | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        if(state.m_stage == ShaderStageFlagBits::GEOMETRY)
            flag = flag | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        if(state.m_stage == ShaderStageFlagBits::COMPUTE)
            flag = flag | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        if(state.m_stage == ShaderStageFlagBits::TASK)
            flag = flag | VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT;
        if(state.m_stage == ShaderStageFlagBits::MESH)
            flag = flag | VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;
    }
    if(uint32_t(state.m_state) & uint32_t(ETextureState::PRESENT))
        flag = flag | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    if(uint32_t(state.m_state) & uint32_t(ETextureState::Undefined))
        flag = flag | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    return flag;
}

VkBufferUsageFlagBits BufferType2VkBufferUsageFlagBits(BufferType type) {
    switch (type) {
    case VERTEX_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    case INDEX_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    case INDIRECT_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    case STAGING_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    case STORAGE_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    case CBV: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    case ACCELERATION_STRUCTURE_STORAGE_BUFFER: return VkBufferUsageFlagBits(
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    case ACCELERATION_STRUCTURE_BUILD_INPUT_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    case NONE: spdlog::error("BufferType2VkBufferUsageFlagBits: NONE buffer type is not supported!"); return VkBufferUsageFlagBits(0);
    default: spdlog::error("BufferType2VkBufferUsageFlagBits: unknown buffer type!"); return VkBufferUsageFlagBits(0);
    }
}

VkShaderStageFlagBits ShaderStageFlagBits2VkShaderStageFlagBits(ShaderStageFlagBits bit) {
    switch (bit) {
    case ShaderStageFlagBits::VERTEX: return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStageFlagBits::FRAGMENT: return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStageFlagBits::GEOMETRY: return VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderStageFlagBits::COMPUTE: return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStageFlagBits::RAYGEN: return VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    case ShaderStageFlagBits::MISS: return VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
    case ShaderStageFlagBits::ANYHIT: return VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    case ShaderStageFlagBits::CLOESTHIT: return VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    case ShaderStageFlagBits::TASK: return VkShaderStageFlagBits::VK_SHADER_STAGE_TASK_BIT_EXT;
    case ShaderStageFlagBits::MESH: return VkShaderStageFlagBits::VK_SHADER_STAGE_MESH_BIT_EXT;
    default: return VkShaderStageFlagBits::VK_SHADER_STAGE_ALL;
    }
}

VkQueueFlagBits QueueType2VkQueueFlagBits(QueueType type) {
    switch (type) {
    case GRAPHICS_QUEUE: return VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
    case COMPUTE_QUEUE: return VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT;
    case TRANSFER_QUEUE: return VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT;
    case PRESENT_QUEUE:return VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
    default: return VK_QUEUE_FLAG_BITS_MAX_ENUM;
    }
}

VkMemoryPropertyFlags BufferType2VkMemoryPropertyFlags(BufferType type) {
    switch (type) {
    case SHADER_INPUT:
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    case STAGING_BUFFER:
        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        //case UNIFORM_BUFFER:
        //    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    default:
        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
}

VkDescriptorType DescriptorType2VkDescriptorType(DescriptorType type) {
    switch (type) {
    case UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case COMBINED_IMAGE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

VkImageAspectFlagBits ETextureState2VkImageAspectFlag(ETextureState state)
{
    switch (state) {
    case ETextureState::DepthAttachment:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

ETextureState VkDescriptorType2ETextureState(VkDescriptorType type)
{
    switch (type) {
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return ETextureState::SRV;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return ETextureState::UAV;
    default:
        spdlog::error("unknown VkDescriptorType:{0}", int(type));
        return ETextureState::Undefined;
    }
}

bool ShaderResourceKey::operator<(const ShaderResourceKey& other) const {
    if (this->set == other.set) {
        return this->binding < other.binding;
    }
    return this->set < other.set;
}

bool ShaderResourceKey::operator==(const ShaderResourceKey& other) const {
    return set == other.set && binding == other.binding;
}