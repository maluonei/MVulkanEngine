#include"MVulkanDevice.h"
#include <stdexcept>
#include<vector>

#include "utils/VulkanUtil.h"

void MVulkanDevice::pickPhysicalDevice(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

QueueFamilyIndices MVulkanDevice::QueryQueueFamilyIndices(VkPhysicalDevice device) {
    auto graphicsIndices = findQueueFamilies(device, QueueType::GRAPHICS_QUEUE);
    auto computeIndices = findQueueFamilies(device, QueueType::COMPUTE_QUEUE);
    auto transferIndices = findQueueFamilies(device, QueueType::TRANSFER_QUEUE);

    QueueFamilyIndices indices;
    indices.graphicsFamily = graphicsIndices;
    indices.computeFamily = computeIndices;
    indices.transferFamily = transferIndices;

    return indices;
}

void MVulkanDevice::createLogicalDevice(VkInstance instance)
{
    indices = QueryQueueFamilyIndices(physicalDevice);

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

    createInfo.enabledExtensionCount = 0;

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
}

bool MVulkanDevice::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = QueryQueueFamilyIndices(device);

    return indices.isComplete();
}

MVulkanDevice::MVulkanDevice()
{

}


uint32_t MVulkanDevice::findQueueFamilies(VkPhysicalDevice device, QueueType type) {
    uint32_t indices = -1;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & QueueType2VkQueueFlagBits(type)) {
            indices = i;
        }

        i++;
    }

    return indices;
}

void MVulkanDevice::Create(VkInstance instance)
{
    pickPhysicalDevice(instance);
    createLogicalDevice(instance);

}

uint32_t MVulkanDevice::GetQueueFamilyIndices(QueueType type) {
    switch (type) {
    case GRAPHICS_QUEUE: return indices.graphicsFamily.value();
    case COMPUTE_QUEUE: return indices.computeFamily.value();
    case TRANSFER_QUEUE: return indices.transferFamily.value();
    default:return -1;
    }
}

VkQueue MVulkanDevice::GetQueue(QueueType type)
{
    switch (type) {
    case GRAPHICS_QUEUE: return graphicsQueue;
    case COMPUTE_QUEUE: return computeQueue;
    case TRANSFER_QUEUE: return transferQueue;
    default: return graphicsQueue;
    }
}
