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
	bool Recreate();

	void Clean();
	VkResult AcquireNextImage(VkSemaphore semephore, VkFence fence, uint32_t* imageIndex);

	inline VkFormat GetSwapChainImageFormat() const { return m_surfaceFormatSRGB.format; }
	inline std::vector<VkImage> GetSwapChainImages() const {
		return m_swapChainImages;
	};

	inline VkImage GetImage(int i)const {
		return m_swapChainImages[i];
	}

	inline std::vector<VkImageView> GetSwapChainImageViews() const {
		return m_swapChainImageViews;
	};

	inline VkImageView GetImageView(int i)const {
		return m_swapChainImageViews[i];
	}

	inline VkExtent2D GetSwapChainExtent() const {
		return m_swapChainExtent;
	}

	inline VkSwapchainKHR Get()const { return m_swapChain; }
	inline VkSwapchainKHR* GetPtr(){ return &m_swapChain; }

	inline uint32_t GetImageCount() const { return static_cast<uint32_t>(m_swapChainImages.size()); }
private:
	MVulkanDevice			m_device;
	VkSurfaceKHR			m_surface;
	GLFWwindow*				m_window;

	VkSwapchainKHR			m_swapChain;

	SwapChainSupportDetails m_swapChainSupport;
	VkSurfaceFormatKHR		m_surfaceFormatSRGB;
	VkSurfaceFormatKHR      m_surfaceFormatUNORM;
	VkPresentModeKHR		m_presentMode;

	std::vector<VkImage>	m_swapChainImages;
	VkExtent2D				m_swapChainExtent;
	std::vector<VkImageView> m_swapChainImageViews;

	void create();
	void createSwapchainImageViews();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat targetFormat);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkExtent2D getCurrentSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};

#endif