#pragma once
#ifndef MVULKANDEVICE_H
#define MVULKANDEVICE_H

#include "vulkan/vulkan_core.h"
#include <optional>

#include "Utils/VulkanUtil.hpp"

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

uint32_t findQueueFamilies(VkPhysicalDevice device, std::vector<VkQueueFamilyProperties>& queueFamilyProperties, QueueType type);

class MVulkanDevice {
public:
    MVulkanDevice();

    inline VkDevice& GetDevice() { return m_logicalDevice; }
    inline VkPhysicalDevice& GetPhysicalDevice() { return m_physicalDevice; }

    void Create(VkInstance instance, VkSurfaceKHR surface);
    void Clean();

    uint32_t GetQueueFamilyIndices(QueueType type);

    VkQueue GetQueue(QueueType type);

    inline VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const{
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);

        return deviceProperties;
    }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat FindDepthFormat();

    inline const VkSampleCountFlagBits GetMaxSmaaFlag() const {
        //return maxSmaaFlag; 
        return VK_SAMPLE_COUNT_1_BIT;
    }

    inline uint32_t GetUniformBufferOffsetAlignment() const {
        return alignment;
    }

    void WaitIdle();

    const inline bool SupportRayTracing() const {
        return m_supportRayTracing;
    }
private:
    VkPhysicalDevice        m_physicalDevice;
    VkDevice                m_logicalDevice;
    QueueFamilyIndices      m_indices;
    VkSampleCountFlagBits   m_maxSmaaFlag;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };

    VkQueue                 m_graphicsQueue;
    VkQueue                 m_computeQueue;
    VkQueue                 m_transferQueue;
    VkQueue                 m_presentQueue;

    uint32_t                alignment = 0;
    bool                    m_supportRayTracing = false;

    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    void createLogicalDevice(VkInstance instance, VkSurfaceKHR surface);

    std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice device);

    QueueFamilyIndices QueryQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkRayracingExtensionSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkSampleCountFlagBits getMaxUsableSampleCount();

    void initRaytracing();
};


#endif

