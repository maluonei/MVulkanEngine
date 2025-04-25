#include "UIRenderer.hpp"

#include "MVulkanRHI/MWindow.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "MVulkanRHI/MVulkanRenderPass.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"

UIRenderer::UIRenderer(
    MWindow* window, 
    MVulkanDevice device, 
    MVulkanInstance instance,
    VkSurfaceKHR surface,
    MVulkanDescriptorSetAllocator allocator,
    std::shared_ptr<MVulkanRenderPass> renderPass,
    int	graphicsQueue)
    :m_window(window), m_device(device), m_instance(instance),
     m_allocator(allocator), m_surface(surface), m_graphicsQueue(graphicsQueue),
    m_renderPass(renderPass)
{

}

void UIRenderer::Init(MVulkanCommandQueue queue)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // 设置 ImGui 样式（可选）
    ImGui::StyleColorsDark();

    // 初始化 GLFW 后端
    ImGui_ImplGlfw_InitForVulkan(m_window->GetWindow(), true);


    //ImGui_ImplGlfw_InitForVulkan(m_window->GetWindow(), true);

    // 初始化 Vulkan 后端
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = m_instance.GetInstance();
    initInfo.PhysicalDevice = m_device.GetPhysicalDevice();
    initInfo.Device = m_device.GetDevice();
    initInfo.QueueFamily = 0; // 你的队列族索引
    initInfo.Queue = queue.Get();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_allocator.Get(); // 需要创建描述符池
    initInfo.MinImageCount = 2; // Swapchain 图像数量
    initInfo.ImageCount = 2;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.RenderPass = m_renderPass->Get();

    ImGui_ImplVulkan_Init(&initInfo);

    // 上传字体纹理（需执行一次 Vulkan 命令）
    //VkCommandBuffer cmd = BeginSingleTimeCommands(device);
    //ImGui_ImplVulkan_CreateFontsTexture(cmd);
    //EndSingleTimeCommands(device, cmd, queue);
	//ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
    
    //cmdList.Reset();
    //cmdList.Begin();

    ImGui_ImplVulkan_CreateFontsTexture();

    //cmdList.End();
    //ImGui_ImplVulkanH_CreateOrResizeWindow(m_instance.GetInstance(), m_device.GetPhysicalDevice(), m_device.GetDevice(), g_MainWindowData, m_graphicsQueue, m_allocator.Get(), width, height, g_MinImageCount);

    
}

void UIRenderer::Update()
{
}

void UIRenderer::SetupVulkanWindow()
{
    //ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    //SetupVulkanWindow(wd, surface, w, h);

    g_MainWindowData.Surface = m_surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_device.GetPhysicalDevice(), m_graphicsQueue, m_surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    g_MainWindowData.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(m_device.GetPhysicalDevice(), g_MainWindowData.Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
//#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
//#else
//    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
//#endif
    g_MainWindowData.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(m_device.GetPhysicalDevice(), g_MainWindowData.Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    //IM_ASSERT(g_MinImageCount >= 2);
    //ImGui_ImplVulkanH_CreateOrResizeWindow(m_instance.GetInstance(), m_device.GetPhysicalDevice(), m_device.GetDevice(), &g_MainWindowData, m_graphicsQueue, m_allocator.Get(), width, height, g_MinImageCount);
}
