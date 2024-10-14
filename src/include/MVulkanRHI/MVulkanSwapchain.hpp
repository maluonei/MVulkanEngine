#pragma once
#ifndef M_VULKANSWAPCHAIN_H
#define M_VULKANSWAPCHAIN_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "MVulkanDevice.hpp"

struct GLFWwindow;

class MVulkanSwapchain {
public:
	MVulkanSwapchain();

	void Create(MVulkanDevice device, GLFWwindow* window, VkSurfaceKHR surface);
	bool Recreate(MVulkanDevice device, GLFWwindow* window, VkSurfaceKHR surface);

	void Clean(VkDevice device);
	VkResult AcquireNextImage(VkDevice device, VkSemaphore semephore, VkFence fence, uint32_t* imageIndex);

	inline VkFormat GetSwapChainImageFormat() const { return surfaceFormat.format; }
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

	SwapChainSupportDetails swapChainSupport;
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;

	std::vector<VkImage> swapChainImages;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	void create(MVulkanDevice device, GLFWwindow* window, VkSurfaceKHR surface);
	void createImageViews(VkDevice device);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
	VkExtent2D getCurrentSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
};

#endif