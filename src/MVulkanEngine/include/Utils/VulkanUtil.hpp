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
};

const std::vector<const char*> raytracingExtensions = {
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
};

struct PipelineVertexInputStateInfo {
    VkVertexInputBindingDescription bindingDesc;
    std::vector<VkVertexInputAttributeDescription> attribDesc;
};

struct MVulkanDescriptorSetLayoutBinding {
    VkDescriptorSetLayoutBinding binding;
    uint32_t size = 0;
};

enum class MVulkanTextureType {
    TEXTURE_2D,
    TEXTURE_CUBEMAP,
};
//struct MDescriptorBufferInfo {
//    VkDescriptorBufferInfo bufferInfo;
//    uint32_t binding;
//};
//
//struct MDescriptorImageInfo {
//    VkDescriptorImageInfo imageInfo;
//    uint32_t binding;
//};

enum QueueType {
    GRAPHICS_QUEUE,
    COMPUTE_QUEUE,
    TRANSFER_QUEUE,
    PRESENT_QUEUE
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
    NONE
};

enum ShaderStageFlagBits {
    VERTEX = 0x01,
    FRAGMENT = 0x02,
    GEOMETRY = 0x04,
    COMPUTE = 0x08,
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
    case VERTEX_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    case INDEX_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    case INDIRECT_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    case STAGING_BUFFER: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
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

    // 检查文件是否成功打开
    if (!file.is_open()) {
        throw std::runtime_error("fail to open file!");
    }

    // 使用 std::ostringstream 读取文件内容到 string
    std::ostringstream buffer;
    buffer << file.rdbuf();  // 将文件流内容复制到缓冲区
    std::string fileContents = buffer.str();  // 转换为 std::string
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

#endif