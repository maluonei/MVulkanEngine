#ifndef MVULKANUTILE_H
#define MVULKANUTILE_H

#include "vector"
#include "vulkan/vulkan_core.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>
#include <cstdint>

#include "spdlog/spdlog.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#define BindlessDescriptorCount 2048

#define VK_CHECK_RESULT(f)                 \
    {                                      \
        VkResult res = (f);                \
                                           \
        if (res != VK_SUCCESS) {           \
            std::stringstream ss;          \
            ss << "Fatal : VkResult is \"" \
               << res                      \
               << "\" in " << __FILE__     \
               << " at line " << __LINE__  \
               << "\n";                    \
            spdlog::error(ss.str());        \
            assert(false);                 \
        }                                  \
    }

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MAINTENANCE3_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    "VK_NV_compute_shader_derivatives",
    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
    //VK_EXT_DEBUG_PRINTF_EXTENSION_NAME
};

const std::vector<const char*> raytracingExtensions = {
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
};

const std::vector<const char*> meshShaderExtensions = {
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_EXT_MESH_SHADER_EXTENSION_NAME,
    VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME
};

struct PipelineVertexInputStateInfo {
    VkVertexInputBindingDescription bindingDesc;
    std::vector<VkVertexInputAttributeDescription> attribDesc;
};


struct RenderingAttachment {
    VkImageView view;
    VkImageLayout layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    glm::vec4 clearColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
    VkClearDepthStencilValue depthStencil = { 1.0f,  0 };
    //VkRenderingAttachmentInfo info;
};

struct RenderingInfo {
    std::vector<RenderingAttachment> colorAttachments;
    RenderingAttachment depthAttachment;
    VkExtent2D extent;
    VkOffset2D offset;
    
    //VkRenderingInfo vkRenderingInfo;
    //void RenderingInfo2VkRenderingInfo();
    //VkRect2D            renderArea;
    //
    //VkRenderingInfo renderInfo;
};

VkRenderingInfo RenderingInfo2VkRenderingInfo(CONST RenderingInfo& info);

//enum class BindingType {
//    Normal,
//    Bindless
//};

struct MVulkanDescriptorSetLayoutBinding {
    VkDescriptorSetLayoutBinding binding;
    uint32_t size = 0;
    //BindingType bindingType = BindingType::Normal;
};

enum class MVulkanTextureType {
    TEXTURE_2D,
    TEXTURE_CUBEMAP,
};

enum QueueType {
    GRAPHICS_QUEUE,
    COMPUTE_QUEUE,
    TRANSFER_QUEUE,
    PRESENT_QUEUE,
};

enum BufferType {
    CBV,
    UAV,
    SRV,
    SHADER_INPUT,
    STAGING_BUFFER,
    INDIRECT_BUFFER,
    VERTEX_BUFFER,
    INDEX_BUFFER,
    STORAGE_BUFFER,
    ACCELERATION_STRUCTURE_STORAGE_BUFFER,
    ACCELERATION_STRUCTURE_BUILD_INPUT_BUFFER,
    NONE
};

enum ShaderStageFlagBits {
    VERTEX = 0x01,
    FRAGMENT = 0x02,
    GEOMETRY = 0x04,
    COMPUTE = 0x08,
    RAYGEN = 0X10,
    MISS = 0X20,
    CLOESTHIT = 0X40,
    ANYHIT = 0X80,
    TASK,
    MESH
};

enum DescriptorType {
    UNIFORM_BUFFER = 0,
    COMBINED_IMAGE_SAMPLER = 1,
    DESCRIPORTYPE_NUM = 2
};

enum AttachmentType {
    COLOR_ATTACHMENT = 0,
    DEPTH_STENCIL_ATTACHMENT = 1
};

inline VkBufferUsageFlagBits BufferType2VkBufferUsageFlagBits(BufferType type) {
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

inline VkShaderStageFlagBits ShaderStageFlagBits2VkShaderStageFlagBits(ShaderStageFlagBits bit) {
    switch (bit) {
    case VERTEX: return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    case FRAGMENT: return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    case GEOMETRY: return VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
    case COMPUTE: return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
    case RAYGEN: return VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    case MISS: return VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
    case ANYHIT: return VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    case CLOESTHIT: return VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    case TASK: return VkShaderStageFlagBits::VK_SHADER_STAGE_TASK_BIT_EXT;
    case MESH: return VkShaderStageFlagBits::VK_SHADER_STAGE_MESH_BIT_EXT;
    default: return VkShaderStageFlagBits::VK_SHADER_STAGE_ALL;
    }
}

inline VkQueueFlagBits QueueType2VkQueueFlagBits(QueueType type) {
    switch (type) {
    case GRAPHICS_QUEUE: return VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
    case COMPUTE_QUEUE: return VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT;
    case TRANSFER_QUEUE: return VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT;
    case PRESENT_QUEUE:return VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
    default: return VK_QUEUE_FLAG_BITS_MAX_ENUM;
    }
}

inline VkMemoryPropertyFlags BufferType2VkMemoryPropertyFlags(BufferType type){
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

inline VkDescriptorType DescriptorType2VkDescriptorType(DescriptorType type) {
    switch (type) {
    case UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case COMBINED_IMAGE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> _buffer(fileSize);

    file.seekg(0);
    file.read(_buffer.data(), fileSize);

    file.close();

    return _buffer;
}

static std::vector<unsigned int> readFileToUnsignedInt(const std::string& filename) {
    std::vector<char> _buffer = readFile(filename);

    std::vector<uint32_t> buffer(_buffer.size());

    std::transform(_buffer.begin(), _buffer.end(), buffer.begin(),
        [](char c) { return static_cast<unsigned int>(static_cast<unsigned char>(c)); });

    return buffer;
}

static std::string readFileToString(const std::string& path) {
    std::ifstream file(path);

    // ����ļ��Ƿ�ɹ���
    if (!file.is_open()) {
        throw std::runtime_error("fail to open file!");
    }

    // ʹ�� std::ostringstream ��ȡ�ļ����ݵ� string
    std::ostringstream buffer;
    buffer << file.rdbuf();  // ���ļ������ݸ��Ƶ�������
    std::string fileContents = buffer.str();  // ת��Ϊ std::string
}

static std::vector<uint32_t> loadSPIRV(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return buffer;
}

static uint32_t CalculateAlignedSize(uint32_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static uint32_t CalculateMipLevels(VkExtent2D extent) {
    int minDim = std::min(extent.width, extent.height);
    return static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(minDim)))) + 1;
}

bool IsDepthFormat(VkFormat format);
#endif