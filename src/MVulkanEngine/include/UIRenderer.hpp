#ifndef UIRENDERER_HPP
#define UIRENDERER_HPP

#include "MVulkanRHI/MVulkanInstance.hpp"
#include "MVulkanRHI/MVulkanDevice.hpp"
#include "MVulkanRHI/MVulkanCommand.hpp"
#include "MVulkanRHI/MVulkanDescriptor.hpp"
#include "imgui_impl_vulkan.h"

class MWindow;
class MVulkanRenderPass;

class UIRenderer {
public:
	UIRenderer() = default;

	UIRenderer(
		MWindow* window,
		MVulkanDevice device,
		MVulkanInstance instance,
		VkSurfaceKHR surface,
		MVulkanDescriptorSetAllocator allocator,
		std::shared_ptr<MVulkanRenderPass> renderPass,
		int	graphicsQueue
	);

	void Init(MVulkanCommandQueue queue);

	void Update();

	void SetupVulkanWindow();

private:
	void createOrResizeWindow();

	//MVulkanRenderPass m_renderPass;
	MVulkanDescriptorSetAllocator m_allocator;
	MVulkanInstance m_instance;
	MVulkanDevice	m_device;
	MWindow*		m_window;
	std::shared_ptr<MVulkanRenderPass> m_renderPass;
	VkSurfaceKHR    m_surface;
	int				m_graphicsQueue;

	ImGui_ImplVulkanH_Window g_MainWindowData;
};

#endif