#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "MVulkanRHI/MVulkanSwapchain.hpp"
#include "MVulkanRHI/MVulkanDevice.hpp"
#include <stdexcept>
#include <algorithm>
#include "spdlog/spdlog.h"

MVulkanSwapchain::MVulkanSwapchain()
{
}

void MVulkanSwapchain::Create(MVulkanDevice device, GLFWwindow* window, VkSurfaceKHR surface)
{
    m_device = device;
    m_surface = surface;

    create();
}

bool MVulkanSwapchain::Recreate()
{
    VkExtent2D swapExtent = getCurrentSwapExtent(m_swapChainSupport.capabilities);
    if (swapExtent.width == m_swapChainExtent.width  && swapExtent.height == m_swapChainExtent.height) {
        return false;
    }

    vkDeviceWaitIdle(m_device.GetDevice());

    m_swapChainExtent = swapExtent;
    
    Clean();
    create();
    return true;
}

void MVulkanSwapchain::Clean()
{
    for (int i = 0; i < m_swapChainImages.size(); i++) {
        vkDestroyImageView(m_device.GetDevice(), m_swapChainImageViews[i], nullptr);
    }
    
    vkDestroySwapchainKHR(m_device.GetDevice(), m_swapChain, nullptr);
    m_swapChainImages.clear();
    m_swapChainImageViews.clear();
}

VkResult MVulkanSwapchain::AcquireNextImage(VkSemaphore semephore, VkFence fence, uint32_t* imageIndex)
{
    return vkAcquireNextImageKHR(m_device.GetDevice(), m_swapChain, UINT64_MAX, semephore, fence, imageIndex);
}


void MVulkanSwapchain::create()
{
    m_swapChainSupport = querySwapChainSupport(m_device.GetPhysicalDevice(), m_surface);
    m_surfaceFormatSRGB = chooseSwapSurfaceFormat(m_swapChainSupport.formats, VK_FORMAT_B8G8R8A8_SRGB);
    //m_surfaceFormatSRGB = chooseSwapSurfaceFormat(m_swapChainSupport.formats, VK_FORMAT_B8G8R8A8_UNORM);
    m_surfaceFormatUNORM = chooseSwapSurfaceFormat(m_swapChainSupport.formats, VK_FORMAT_B8G8R8A8_UNORM);
    m_presentMode = chooseSwapPresentMode(m_swapChainSupport.presentModes);
    m_swapChainExtent = chooseSwapExtent(m_swapChainSupport.capabilities);

    uint32_t imageCount = m_swapChainSupport.capabilities.minImageCount + 1;
    if (m_swapChainSupport.capabilities.maxImageCount > 0 && imageCount > m_swapChainSupport.capabilities.maxImageCount) {
        imageCount = m_swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = m_surfaceFormatSRGB.format;
    createInfo.imageColorSpace = m_surfaceFormatSRGB.colorSpace;
    createInfo.imageExtent = m_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t queueFamilyIndices[] = { m_device.GetQueueFamilyIndices(QueueType::GRAPHICS_QUEUE), m_device.GetQueueFamilyIndices(QueueType::PRESENT_QUEUE) };

    if (m_device.GetQueueFamilyIndices(QueueType::GRAPHICS_QUEUE) != m_device.GetQueueFamilyIndices(QueueType::PRESENT_QUEUE)) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = m_swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = m_presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device.GetDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(m_device.GetDevice(), m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    spdlog::info("imageCount:{0}", imageCount);
    vkGetSwapchainImagesKHR(m_device.GetDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

    createSwapchainImageViews();
}

void MVulkanSwapchain::createSwapchainImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_surfaceFormatSRGB.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK_RESULT(vkCreateImageView(m_device.GetDevice(), &createInfo, nullptr, &m_swapChainImageViews[i]));
    }
}

VkSurfaceFormatKHR MVulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat targetFormat) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == targetFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }

        //if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        //    return availableFormat;
        //}
        //if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        //    return availableFormat;
        //}
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

VkExtent2D MVulkanSwapchain::getCurrentSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    int width = 0, height = 0;

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwPollEvents();
    }

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    return actualExtent;
}

VkExtent2D MVulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D actualExtent = getCurrentSwapExtent( capabilities);
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}