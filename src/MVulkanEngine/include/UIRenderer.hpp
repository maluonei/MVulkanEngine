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

	void Render();

	void RenderFrame(MVulkanCommandList commandList);
	void PresentFrame();

	bool IsRender();

	VkSemaphore GetImageRequireSemaphore(int imageIndex);
	VkSemaphore GetRenderCompleteSemaphore(int imageIndex);
	VkFence GetFence(int imageIndex);

	VkRenderPassBeginInfo GetRenderPassBeginInfo();
	RenderingInfo GetRenderingInfo();

	VkExtent2D GetRenderingExtent();

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

	bool show_demo_window = true;
	bool show_another_window = false;
	bool render = true;

	glm::vec4 clear_color = glm::vec4(0.f, 0.f, 0.f, 1.f);
};

#endif