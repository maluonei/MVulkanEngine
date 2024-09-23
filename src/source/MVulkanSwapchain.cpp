#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "MVulkanSwapchain.h"
#include "MVulkanDevice.h"
#include <stdexcept>
#include <algorithm>
#include "spdlog/spdlog.h"

MVulkanSwapchain::MVulkanSwapchain()
{
}

void MVulkanSwapchain::Create(MVulkanDevice device, GLFWwindow* window, VkSurfaceKHR surface)
{
    swapChainSupport = querySwapChainSupport(device.GetPhysicalDevice(), surface);
    surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    swapChainExtent = chooseSwapExtent(window, swapChainSupport.capabilities);

    create(device, window, surface);
}

bool MVulkanSwapchain::Recreate(MVulkanDevice device, GLFWwindow* window, VkSurfaceKHR surface)
{
    VkExtent2D swapExtent = getCurrentSwapExtent(window, swapChainSupport.capabilities);
    if (swapExtent.width == swapChainExtent.width  && swapExtent.height == swapChainExtent.height) {
        return false;
    }

    vkDeviceWaitIdle(device.GetDevice());

    swapChainExtent = swapExtent;
    
    Clean(device.GetDevice());
    Create(device, window, surface);
    return true;
}

void MVulkanSwapchain::Clean(VkDevice device)
{
    swapChainImages.clear();
    swapChainImageViews.clear();

    for (int i = 0; i < swapChainImages.size(); i++) {
        vkDestroyImage(device, swapChainImages[i], nullptr);
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

VkResult MVulkanSwapchain::AcquireNextImage(VkDevice device, VkSemaphore semephore, VkFence fence, uint32_t* imageIndex)
{
    return vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, semephore, fence, imageIndex);
}


void MVulkanSwapchain::create(MVulkanDevice device, GLFWwindow* window, VkSurfaceKHR surface)
{
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    //uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    uint32_t queueFamilyIndices[] = { device.GetQueueFamilyIndices(QueueType::GRAPHICS_QUEUE),device.GetQueueFamilyIndices(QueueType::PRESENT_QUEUE) };

    if (device.GetQueueFamilyIndices(QueueType::GRAPHICS_QUEUE) != device.GetQueueFamilyIndices(QueueType::PRESENT_QUEUE)) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device.GetDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device.GetDevice(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    spdlog::info("imageCount:{0}", imageCount);
    vkGetSwapchainImagesKHR(device.GetDevice(), swapChain, &imageCount, swapChainImages.data());

    createImageViews(device.GetDevice());
}

void MVulkanSwapchain::createImageViews(VkDevice device)
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = surfaceFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

VkSurfaceFormatKHR MVulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR MVulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D MVulkanSwapchain::getCurrentSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
{
    int width = 0, height = 0;

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwPollEvents();
    }

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    return actualExtent;
}

VkExtent2D MVulkanSwapchain::chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D actualExtent = getCurrentSwapExtent(window, capabilities);
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}