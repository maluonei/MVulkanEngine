#ifndef MVULKANUTILE_H
#define MVULKANUTILE_H

#include "vector"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

enum QueueType {
    GRAPHICS_QUEUE,
    COMPUTE_QUEUE,
    TRANSFER_QUEUE
};

inline VkQueueFlagBits QueueType2VkQueueFlagBits(QueueType type) {
    switch (type) {
    case GRAPHICS_QUEUE: return VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
    case COMPUTE_QUEUE: return VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT;
    case TRANSFER_QUEUE: return VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT;
    default: return VK_QUEUE_FLAG_BITS_MAX_ENUM;
    }
}

#endif