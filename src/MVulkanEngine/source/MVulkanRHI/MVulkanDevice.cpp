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
            m_physicalDevice = device;
            m_maxSmaaFlag = getMaxUsableSampleCount();
            
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;

            spdlog::info("picked gpu is {0}", deviceProperties.deviceName);
            spdlog::info("Vulkan API Version:{0}.{1}.{2}", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion));
            spdlog::info("maxSmaaFlag:{0}", int(m_maxSmaaFlag));
            spdlog::info("VkPhysicalDeviceLimits.maxDrawIndirectCount:{0}", deviceProperties.limits.maxDrawIndirectCount);
            spdlog::info("VkPhysicalDeviceLimits.maxDrawIndexedIndexValue:{0}", deviceProperties.limits.maxDrawIndexedIndexValue);
            spdlog::info("Max descriptor set sampled images:{0}", static_cast<int>(deviceProperties.limits.maxDescriptorSetSampledImages));
            spdlog::info("deviceProperties.limits.maxPushConstantsSize:{0}", static_cast<int>(deviceProperties.limits.maxPushConstantsSize));
            spdlog::info("deviceProperties.limits.minUniformBufferOffsetAlignment:{0}", static_cast<int>(deviceProperties.limits.minUniformBufferOffsetAlignment));
            spdlog::info("deviceProperties.limits.maxUniformBufferRange:{0}", static_cast<int>(deviceProperties.limits.maxUniformBufferRange));
            spdlog::info("deviceProperties.limits.maxStorageBufferRange:{0}", deviceProperties.limits.maxStorageBufferRange);
            spdlog::info("deviceProperties.limits.maxComputeWorkGroupSize:{0},{1},{2}", 
                static_cast<int>(deviceProperties.limits.maxComputeWorkGroupSize[0]),
                static_cast<int>(deviceProperties.limits.maxComputeWorkGroupSize[1]), 
                static_cast<int>(deviceProperties.limits.maxComputeWorkGroupSize[2]));
            spdlog::info("device.alignment: {0}", alignment);
            
            if (m_supportRayTracing) {
                spdlog::info("device support raytracing");
                initRaytracing();
            }
            else {
                spdlog::warn("device don't support raytracing");
            }

            if (m_supportMeshShader) {
                spdlog::info("device support mesh shader");
                initMeshShaderFeatures();
            }
            else {
                spdlog::warn("device don't support mesh shader");
            }
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
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
    m_indices = QueryQueueFamilyIndices(m_physicalDevice, surface);

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(3, queueCreateInfo);
    queueCreateInfos[1].queueFamilyIndex = m_indices.computeFamily.value();
    queueCreateInfos[2].queueFamilyIndex = m_indices.transferFamily.value();

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
    meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    meshShaderFeatures.meshShader = VK_TRUE;
    meshShaderFeatures.taskShader = VK_TRUE; // 任务着色器可选

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery = VK_TRUE;
    //rayQueryFeatures.pNext = &meshShaderFeatures;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {};
    rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingFeatures.rayTracingPipeline = VK_TRUE;
    rayTracingFeatures.pNext = &rayQueryFeatures;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;
    //accelerationStructureFeatures.accelerationStructureHostCommands = VK_TRUE;
    accelerationStructureFeatures.pNext = &rayTracingFeatures;

    VkPhysicalDeviceBufferDeviceAddressFeatures deviceAddressFeatures = {};
    deviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    deviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    if (m_supportRayTracing) {
        if (m_supportMeshShader) {
            rayQueryFeatures.pNext = &meshShaderFeatures;
        }
        deviceAddressFeatures.pNext = &accelerationStructureFeatures;
    }
    //VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
    //meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    else if (m_supportMeshShader) {
        deviceAddressFeatures.pNext = &meshShaderFeatures;
    }

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
    descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
    descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeatures.pNext = &deviceAddressFeatures;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.multiDrawIndirect = VK_TRUE;
    deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
    deviceFeatures.geometryShader = VK_TRUE;

    VkPhysicalDeviceComputeShaderDerivativesFeaturesNV physicalDeviceComputeShaderDerivativesFeatures{};
    physicalDeviceComputeShaderDerivativesFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV;
    physicalDeviceComputeShaderDerivativesFeatures.computeDerivativeGroupQuads = VK_TRUE;
    physicalDeviceComputeShaderDerivativesFeatures.computeDerivativeGroupLinear = VK_TRUE;
    physicalDeviceComputeShaderDerivativesFeatures.pNext = &descriptorIndexingFeatures;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &physicalDeviceComputeShaderDerivativesFeatures;
    deviceFeatures2.features = deviceFeatures;



    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    //createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.pEnabledFeatures = nullptr;

    std::vector<const char*> enabledExtensionNames = deviceExtensions;
    if (m_supportRayTracing) {
        enabledExtensionNames.insert(enabledExtensionNames.end(), raytracingExtensions.begin(), raytracingExtensions.end());
    } 
    if (m_supportMeshShader) {
        enabledExtensionNames.insert(enabledExtensionNames.end(), meshShaderExtensions.begin(), meshShaderExtensions.end());
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size());
    createInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

    createInfo.pNext = &deviceFeatures2;

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }


    vkGetDeviceQueue(m_logicalDevice, m_indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_logicalDevice, m_indices.computeFamily.value(), 0, &m_computeQueue);
    vkGetDeviceQueue(m_logicalDevice, m_indices.transferFamily.value(), 0, &m_transferQueue);
    vkGetDeviceQueue(m_logicalDevice, m_indices.presentFamily.value(), 0, &m_presentQueue);
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

bool MVulkanDevice::checkRayracingExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(raytracingExtensions.begin(), raytracingExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool MVulkanDevice::checkMeshShaderExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(meshShaderExtensions.begin(), meshShaderExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
    meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &meshShaderFeatures;

    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

    //if (!meshShaderFeatures.meshShader) {
    //    throw std::runtime_error("Mesh Shader is not supported on this device!");
    //}

    return requiredExtensions.empty() && meshShaderFeatures.meshShader;
}

bool MVulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = QueryQueueFamilyIndices(device, surface); 

    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool rayTracingSupported = checkRayracingExtensionSupport(device);
    bool meshShaderSupported = checkMeshShaderExtensionSupport(device);

    if (rayTracingSupported) m_supportRayTracing = true;
    if (meshShaderSupported) m_supportMeshShader = true;

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
    vkDeviceWaitIdle(m_logicalDevice);
}

VkFormat MVulkanDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

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
    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

void MVulkanDevice::initRaytracing()
{
    VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    prop2.pNext = &m_rtProperties;
    vkGetPhysicalDeviceProperties2(m_physicalDevice, &prop2);

    spdlog::info("maxRayRecursionDepth:{0}", m_rtProperties.maxRayRecursionDepth);
    spdlog::info("shaderGroupBaseAlignment:{0}", m_rtProperties.shaderGroupBaseAlignment);
    spdlog::info("maxRayDispatchInvocationCount:{0}", m_rtProperties.maxRayDispatchInvocationCount);
    spdlog::info("maxRayHitAttributeSize:{0}", m_rtProperties.maxRayHitAttributeSize);
}

void MVulkanDevice::initMeshShaderFeatures()
{

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
    vkDestroyDevice(m_logicalDevice, nullptr);
}

uint32_t MVulkanDevice::GetQueueFamilyIndices(QueueType type) {
    switch (type) {
    case GRAPHICS_QUEUE: return m_indices.graphicsFamily.value();
    case COMPUTE_QUEUE: return m_indices.computeFamily.value();
    case TRANSFER_QUEUE: return m_indices.transferFamily.value();
    case PRESENT_QUEUE:return m_indices.presentFamily.value();
    default:return -1;
    }
}

VkQueue MVulkanDevice::GetQueue(QueueType type)
{
    switch (type) {
    case GRAPHICS_QUEUE: return m_graphicsQueue;
    case COMPUTE_QUEUE: return m_computeQueue;
    case TRANSFER_QUEUE: return m_transferQueue;
    case PRESENT_QUEUE: return m_presentQueue;
    default: return m_graphicsQueue;
    }
}

uint32_t MVulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
