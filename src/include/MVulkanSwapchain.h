#pragma once
#ifndef M_VULKANSWAPCHAIN_H
#define M_VULKANSWAPCHAIN_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "MVulkanDevice.h"

class GLFWwindow;

class MVulkanSwapchain {
public:
	MVulkanSwapchain();

	void Create(MVulkanDevice device, GLFWwindow* window, VkSurfaceKHR surface);
	void Clean(VkDevice device);

	inline VkFormat GetSwapChainImageFormat() const { return swapChainImageFormat; }
	inline std::vector<VkImage> GetSwapChainImages() const {
		return swapChainImages;
	};
	inline std::vector<VkImageView> GetSwapChainImageViews() const {
		return swapChainImageViews;
	};
	inline VkExtent2D GetSwapChainExtent() const {
		return swapChainExtent;
	}

	inline VkSwapchainKHR Get()const { return swapChain; }
private:
	VkSwapchainKHR swapChain;

	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	void createImageViews(VkDevice device);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
};

#endif