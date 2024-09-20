#include"MVulkanDevice.h"
#include <stdexcept>
#include<vector>
#include<set>
#include<string>
#include "spdlog/spdlog.h"

#include "utils/VulkanUtil.h"


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
            
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            spdlog::info("picked gpu is {0}", deviceProperties.deviceName);

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

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
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
