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

enum ResourceType
{
    ResourceType_ConstantBuffer,
    ResourceType_StorageBuffer,
    ResourceType_SampledImage,
    ResourceType_StorageImage,
    ResourceType_Sampler,
    ResourceType_CombinedImageSampler,
    ResourceType_AccelerationStructure
};

VkDescriptorType ResourceType2VkDescriptorType(const ResourceType type);

struct MVulkanImageMemoryBarrier {
    VkAccessFlags              srcAccessMask = 0;
    VkAccessFlags              dstAccessMask = 0;
    VkImageLayout              oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout              newLayout = VK_IMAGE_LAYOUT_GENERAL;
    uint32_t                   srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uint32_t                   dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    VkImage                    image;
    VkImageAspectFlags		   aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t				   baseMipLevel = 0;
    uint32_t				   levelCount = 1;
    uint32_t				   baseArrayLayer = 0;
    uint32_t				   layerCount = 1;
};

class MVulkanTexture;
struct RenderingAttachment {
    std::shared_ptr<MVulkanTexture> texture = nullptr;
    VkImageLayout layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    //for swapchain image
    VkImageView view;

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
    bool useDepth = true;
    
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
    std::string name = "";
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

enum class ShaderStageFlagBits {
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

enum class ETextureState : uint32_t {
    Undefined = 1 << 0,
    ColorAttachment = 1 << 1,
    DepthAttachment = 1 << 2,
    SRV = 1 << 3,
    UAV = 1 << 4,
    PRESENT = 1 << 5,
    GENERAL = 1 << 6
};

struct TextureState {
    ETextureState m_state = ETextureState::Undefined;
    ShaderStageFlagBits m_stage = ShaderStageFlagBits::VERTEX;
};

VkImageLayout TextureState2ImageLayout(const ETextureState state);

VkAccessFlagBits TextureState2AccessFlag(const ETextureState state);

ShaderStageFlagBits VkShaderStage2ShaderStage(const VkShaderStageFlags stage);
//ETextureState ImageLayout2TextureState(const VkImageLayout layout);

VkPipelineStageFlags TextureState2PipelineStage(const TextureState state);

VkBufferUsageFlagBits BufferType2VkBufferUsageFlagBits(BufferType type);

VkShaderStageFlagBits ShaderStageFlagBits2VkShaderStageFlagBits(ShaderStageFlagBits bit);

VkQueueFlagBits QueueType2VkQueueFlagBits(QueueType type);

VkMemoryPropertyFlags BufferType2VkMemoryPropertyFlags(BufferType type);

VkDescriptorType DescriptorType2VkDescriptorType(DescriptorType type);

VkImageAspectFlagBits ETextureState2VkImageAspectFlag(ETextureState state);

ETextureState VkDescriptorType2ETextureState(VkDescriptorType type);

struct ShaderResourceKey {
    int set;
    int binding;
    bool operator<(const ShaderResourceKey& other) const;

    bool operator==(const ShaderResourceKey& other) const;
};

namespace std {
    template <>
    struct hash<ShaderResourceKey> {
        std::size_t operator()(const ShaderResourceKey& k) const {
            return std::hash<int>()(k.set) ^ (std::hash<int>()(k.binding) << 1);
        }
    };
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