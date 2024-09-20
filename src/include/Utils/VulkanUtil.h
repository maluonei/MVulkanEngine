#ifndef MVULKANUTILE_H
#define MVULKANUTILE_H

#include "vector"
#include "vulkan/vulkan_core.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

#include "spdlog/spdlog.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

enum QueueType {
    GRAPHICS_QUEUE,
    COMPUTE_QUEUE,
    TRANSFER_QUEUE,
    PRESENT_QUEUE
};

inline VkQueueFlagBits QueueType2VkQueueFlagBits(QueueType type) {
    switch (type) {
    case GRAPHICS_QUEUE: return VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
    case COMPUTE_QUEUE: return VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT;
    case TRANSFER_QUEUE: return VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT;
    case PRESENT_QUEUE:return VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
    default: return VK_QUEUE_FLAG_BITS_MAX_ENUM;
    }
}

enum ShaderStageFlagBits {
    VERTEX = 0x01,
    FRAGMENT = 0x02,
    GEOMETRY = 0x04,
    COMPUTE = 0x08,
};

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
        //std::cerr << "Unable to open file!" << std::endl;
        //spdlog::error("fail to open file: {}", path);
        throw std::runtime_error("fail to open file!");
    }

    // 使用 std::ostringstream 读取文件内容到 string
    std::ostringstream buffer;
    buffer << file.rdbuf();  // 将文件流内容复制到缓冲区
    std::string fileContents = buffer.str();  // 转换为 std::string
}

#endif