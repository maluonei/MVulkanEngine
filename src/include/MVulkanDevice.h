#pragma once
#ifndef MVULKANDEVICE_H
#define MVULKANDEVICE_H

#include "vulkan/vulkan_core.h"
#include <optional>

#include "Utils/VulkanUtil.h"

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
    }
};


class MVulkanDevice {
public:
    MVulkanDevice();

    inline VkDevice& GetDevice() { return logicalDevice; }

    void Create(VkInstance instance);

    uint32_t GetQueueFamilyIndices(QueueType type);

    VkQueue GetQueue(QueueType type);

private:
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    QueueFamilyIndices indices;

    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue transferQueue;

    void pickPhysicalDevice(VkInstance instance);
    void createLogicalDevice(VkInstance instance);

    uint32_t findQueueFamilies(VkPhysicalDevice device, QueueType type);
    QueueFamilyIndices QueryQueueFamilyIndices(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);
};


#endif

