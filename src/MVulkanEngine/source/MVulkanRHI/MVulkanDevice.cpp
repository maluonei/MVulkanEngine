#include"MVulkanRHI/MVulkanDevice.hpp"
#include <stdexcept>
#include<vector>
#include<set>
#include<string>
#include "spdlog/spdlog.h"

#include "utils/VulkanUtil.hpp"


SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}


uint32_t findQueueFamilies(VkPhysicalDevice device, std::vector<VkQueueFamilyProperties>& queueFamilyProperties, QueueType type) {
    uint32_t indices = -1;

    int i = 0;
    for (const auto& queueFamily : queueFamilyProperties) {
        if (queueFamily.queueFlags & QueueType2VkQueueFlagBits(type)) {
            indices = i;
        }

        i++;
    }

    return indices;
}

void MVulkanDevice::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device, surface)) {
            physicalDevice = device;
            maxSmaaFlag = getMaxUsableSampleCount();
            
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;

            spdlog::info("picked gpu is {0}", deviceProperties.deviceName);
            spdlog::info("Vulkan API Version:{0}.{1}.{2}", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion));
            spdlog::info("maxSmaaFlag:{0}", int(maxSmaaFlag));
            spdlog::info("VkPhysicalDeviceLimits.maxDrawIndirectCount:{0}", static_cast<int>(deviceProperties.limits.maxDrawIndirectCount));
            spdlog::info("VkPhysicalDeviceLimits.maxDrawIndexedIndexValue:{0}", static_cast<int>(deviceProperties.limits.maxDrawIndexedIndexValue));
            spdlog::info("Max descriptor set sampled images:{0}", static_cast<int>(deviceProperties.limits.maxDescriptorSetSampledImages));
            spdlog::info("deviceProperties.limits.maxPushConstantsSize:{0}", static_cast<int>(deviceProperties.limits.maxPushConstantsSize));
            spdlog::info("deviceProperties.limits.minUniformBufferOffsetAlignment:{0}", static_cast<int>(deviceProperties.limits.minUniformBufferOffsetAlignment));
            spdlog::info("device.alignment: {0}", alignment);
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

QueueFamilyIndices MVulkanDevice::QueryQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::vector<VkQueueFamilyProperties> queueFamilies = GetQueueFamilyProperties(device);

    auto graphicsIndices = findQueueFamilies(device, queueFamilies, QueueType::GRAPHICS_QUEUE);
    auto computeIndices = findQueueFamilies(device, queueFamilies, QueueType::COMPUTE_QUEUE);
    auto transferIndices = findQueueFamilies(device, queueFamilies, QueueType::TRANSFER_QUEUE);

    QueueFamilyIndices indices;
    indices.graphicsFamily = graphicsIndices;
    indices.computeFamily = computeIndices;
    indices.transferFamily = transferIndices;

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
            break;
        }

        i++;
    }

    return indices;
}

void MVulkanDevice::createLogicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    indices = QueryQueueFamilyIndices(physicalDevice, surface);

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(3, queueCreateInfo);
    queueCreateInfos[1].queueFamilyIndex = indices.computeFamily.value();
    queueCreateInfos[2].queueFamilyIndex = indices.transferFamily.value();

    //VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {};
    //descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    //descriptorIndexingFeatures.nullDescriptor = VK_TRUE;  // ���� nullDescriptor ����

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
    descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
    descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.multiDrawIndirect = VK_TRUE;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &descriptorIndexingFeatures;
    deviceFeatures2.features = deviceFeatures;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    //createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.pEnabledFeatures = nullptr;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    createInfo.pNext = &deviceFeatures2;

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, indices.computeFamily.value(), 0, &computeQueue);
    vkGetDeviceQueue(logicalDevice, indices.transferFamily.value(), 0, &transferQueue);
    vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
}

std::vector<VkQueueFamilyProperties> MVulkanDevice::GetQueueFamilyProperties(VkPhysicalDevice device)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    return queueFamilies;
}

bool MVulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        //spdlog::info(extension.extensionName);
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool MVulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = QueryQueueFamilyIndices(device, surface); 

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy && supportedFeatures.multiDrawIndirect;
}

VkFormat MVulkanDevice::FindDepthFormat()
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void MVulkanDevice::WaitIdle()
{
    vkDeviceWaitIdle(logicalDevice);
}

VkFormat MVulkanDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkSampleCountFlagBits MVulkanDevice::getMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

MVulkanDevice::MVulkanDevice()
{

}

void MVulkanDevice::Create(VkInstance instance, VkSurfaceKHR surface)
{
    pickPhysicalDevice(instance, surface);
    createLogicalDevice(instance, surface);
}

void MVulkanDevice::Clean()
{
    vkDestroyDevice(logicalDevice, nullptr);
}

uint32_t MVulkanDevice::GetQueueFamilyIndices(QueueType type) {
    switch (type) {
    case GRAPHICS_QUEUE: return indices.graphicsFamily.value();
    case COMPUTE_QUEUE: return indices.computeFamily.value();
    case TRANSFER_QUEUE: return indices.transferFamily.value();
    case PRESENT_QUEUE:return indices.presentFamily.value();
    default:return -1;
    }
}

VkQueue MVulkanDevice::GetQueue(QueueType type)
{
    switch (type) {
    case GRAPHICS_QUEUE: return graphicsQueue;
    case COMPUTE_QUEUE: return computeQueue;
    case TRANSFER_QUEUE: return transferQueue;
    case PRESENT_QUEUE: return presentQueue;
    default: return graphicsQueue;
    }
}

uint32_t MVulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}