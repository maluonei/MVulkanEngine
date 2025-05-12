#include "UIRenderer.hpp"

#include "MVulkanRHI/MWindow.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "MVulkanRHI/MVulkanRenderPass.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"

void UIRenderer::Init(
    MWindow* window, 
    MVulkanDevice device, 
    MVulkanInstance instance,
    VkSurfaceKHR surface,
    MVulkanDescriptorSetAllocator allocator,
    int	graphicsQueue,
    MRenderApplication* app)
{
    m_window = window;
    m_device = device;
    m_instance = instance;
    m_allocator = allocator;
    m_surface = surface;
    m_graphicsQueue = graphicsQueue;
    m_app = app;
}

void UIRenderer::InitImgui(MVulkanCommandQueue queue)
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
    initInfo.MinImageCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount(); // Swapchain 图像数量
    initInfo.ImageCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    //initInfo.RenderPass = m_renderPass->Get();
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat swapchainFormat = Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat();
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainFormat;

    ImGui_ImplVulkan_Init(&initInfo);

    // 上传字体纹理（需执行一次 Vulkan 命令）
    //VkCommandBuffer cmd = BeginSingleTimeCommands(device);
    //ImGui_ImplVulkan_CreateFontsTexture(cmd);
    //EndSingleTimeCommands(device, cmd, queue);
	//ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
    
    //cmdList.Reset();
    //cmdList.Begin();

    ImGui_ImplVulkan_CreateFontsTexture();

    ImGui::CreateContext();

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

void UIRenderer::Render()
{
    ImGuiIO& io = ImGui::GetIO();

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    //if (show_demo_window)
    //    ImGui::ShowDemoWindow(&show_demo_window);
    //
    //// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    //{
    //    static float f = 0.0f;
    //    static int counter = 0;
    //
    //    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
    //
    //    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    //    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    //    ImGui::Checkbox("Another Window", &show_another_window);
    //
    //    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    //    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
    //
    //    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
    //        counter++;
    //    ImGui::SameLine();
    //    ImGui::Text("counter = %d", counter);
    //
    //    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    //    ImGui::End();
    //}
    //
    //// 3. Show another simple window.
    //if (show_another_window)
    //{
    //    ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    //    ImGui::Text("Hello from another window!");
    //    if (ImGui::Button("Close Me"))
    //        show_another_window = false;
    //    ImGui::End();
    //}

    RenderContext();

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        g_MainWindowData.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
        g_MainWindowData.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
        g_MainWindowData.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
        g_MainWindowData.ClearValue.color.float32[3] = clear_color.w;
        //FrameRender(g_MainWindowData, draw_data);
        //FramePresent(g_MainWindowData);
    }
}

void UIRenderer::RenderFrame(MVulkanCommandList commandList)
{
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandList.GetBuffer());
}

bool UIRenderer::IsRender()
{
    return render;
}

VkSemaphore UIRenderer::GetImageRequireSemaphore(int imageIndex)
{
    return g_MainWindowData.FrameSemaphores[g_MainWindowData.SemaphoreIndex].ImageAcquiredSemaphore;
}

VkSemaphore UIRenderer::GetRenderCompleteSemaphore(int imageIndex)
{
    return g_MainWindowData.FrameSemaphores[g_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;
}

VkFence UIRenderer::GetFence(int imageIndex)
{
    return g_MainWindowData.Frames[g_MainWindowData.FrameIndex].Fence;
}

//VkRenderPassBeginInfo UIRenderer::GetRenderPassBeginInfo()
//{
//    ImGui_ImplVulkanH_Frame* fd = &g_MainWindowData.Frames[g_MainWindowData.FrameIndex];
//
//    VkRenderPassBeginInfo renderPassInfo{};
//    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//    renderPassInfo.renderPass = m_renderPass->Get();
//    renderPassInfo.framebuffer = fd->Framebuffer;
//    renderPassInfo.renderArea.offset = { 0, 0 };
//    renderPassInfo.renderArea.extent = { (unsigned int)g_MainWindowData.Width, (unsigned int)g_MainWindowData.Height};
//
//    //uint32_t attachmentCount = renderPass->GetAttachmentCount();
//    //std::vector<VkClearValue> clearValues(attachmentCount + 1);
//    //for (auto i = 0; i < attachmentCount; i++) {
//    //    clearValues[i].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
//    //}
//    //clearValues[attachmentCount].depthStencil = { 1.0f, 0 };
//    VkClearValue clearValue{};
//    clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
//
//    renderPassInfo.clearValueCount = 1;
//    renderPassInfo.pClearValues = &clearValue;
//
//    return renderPassInfo;
//}
//
//RenderingInfo UIRenderer::GetRenderingInfo()
//{
//    RenderingInfo renderingInfo{};
//
//
//
//    return RenderingInfo();
//}

VkExtent2D UIRenderer::GetRenderingExtent()
{
    return {(uint32_t)g_MainWindowData.Width, (uint32_t)g_MainWindowData.Height};
}

void UIRenderer::RenderContext()
{
}
