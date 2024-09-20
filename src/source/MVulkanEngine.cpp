#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"MWindow.h"
#include"MVulkanEngine.h"
#include <stdexcept>

#include "MVulkanShader.h"

MVulkanEngine::MVulkanEngine()
{
    window = new MWindow();
}

MVulkanEngine::~MVulkanEngine()
{
    delete window;
}

void MVulkanEngine::Init()
{
    window->Init(windowWidth, windowHeight);

    initVulkan();
}

void MVulkanEngine::Clean()
{
    window->Clean();

    //SpirvHelper::Finalize();

    commandAllocator.Clean(device.GetDevice());

    for (auto i = 0; i < frameBuffers.size(); i++) {
        frameBuffers[i].Clean(device.GetDevice());
    }

    swapChain.Clean(device.GetDevice());

    pipeline.Clean(device.GetDevice());
    renderPass.Clean(device.GetDevice());

    device.Clean();
    vkDestroySurfaceKHR(instance.GetInstance(), surface, nullptr);
    instance.Clean();
}

void MVulkanEngine::Run()
{
    while (!window->WindowShouldClose()) {
        renderLoop();
    }
}

void MVulkanEngine::SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight)
{
    windowWidth = _windowWidth;
    windowHeight = _windowHeight;
}

void MVulkanEngine::initVulkan()
{
    createInstance();
    createSurface();
    createDevice();
    createSwapChain();
    createRenderPass();
    createPipeline();
    createFrameBuffers();
    createCommandQueue();
    createCommandAllocator();
    createCommandList();

    createBufferAndLoadData();
}

void MVulkanEngine::renderLoop()
{

    window->WindowPollEvents();
}

void MVulkanEngine::createInstance()
{
    instance.Create();
    instance.SetupDebugMessenger();
}

void MVulkanEngine::createDevice()
{
    device.Create(instance.GetInstance(), surface);
}

void MVulkanEngine::createSurface()
{
    if (glfwCreateWindowSurface(instance.GetInstance(), window->GetWindow(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void MVulkanEngine::createSwapChain()
{
    swapChain.Create(device, window->GetWindow(), surface);
}

void MVulkanEngine::createRenderPass()
{
    renderPass.Create(device.GetDevice(), swapChain.GetSwapChainImageFormat());
}

void MVulkanEngine::createPipeline()
{
    std::string vertPath = "test.vert.glsl";
    std::string fragPath = "test.frag.glsl";

    MVulkanShader vert(vertPath, ShaderStageFlagBits::VERTEX);
    MVulkanShader frag(fragPath, ShaderStageFlagBits::FRAGMENT);

    vert.Create(device.GetDevice());
    frag.Create(device.GetDevice());

    MVulkanShaderReflector reflector(vert.GetShader());
    PipelineVertexInputStateInfo info = reflector.GenerateVertexInputAttributes();

    pipeline.Create(device.GetDevice(), vert.GetShaderModule(), frag.GetShaderModule(), renderPass.Get(), info);

    vert.Clean(device.GetDevice());
    frag.Clean(device.GetDevice());
}

void MVulkanEngine::createFrameBuffers()
{
    std::vector<VkImageView> imageViews = swapChain.GetSwapChainImageViews();

    frameBuffers.resize(imageViews.size());
    for (auto i = 0; i < imageViews.size(); i++) {
        FrameBufferCreateInfo info;

        info.attachments = &imageViews[i];
        info.numAttachments = 1;
        info.renderPass = renderPass.Get();
        info.extent = swapChain.GetSwapChainExtent();

        frameBuffers[i].Create(device.GetDevice(), info);
    }
}

void MVulkanEngine::createCommandQueue()
{
    graphicsQueue.SetQueue(device.GetQueue(QueueType::GRAPHICS_QUEUE));
}

void MVulkanEngine::createCommandAllocator()
{
    commandAllocator.Create(device);
}

void MVulkanEngine::createCommandList()
{
    VkCommandListCreateInfo info;
    info.commandPool = commandAllocator.Get(QueueType::GRAPHICS_QUEUE);
    info.commandBufferCount = 1;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    commandList.Create(device.GetDevice(), info);
}

void MVulkanEngine::createBufferAndLoadData()
{
    float verteices[] = {
        0.f, 1.f, 0.f,
        -1.f, -1.f, 0.f,
        1.f, -1.f, 0.f
    };

    BufferCreateInfo info;
    info.size = sizeof(verteices);
    
    commandList.Begin();
    vertexBuffer.CreateAndLoadData(commandList, device, info, verteices);
    commandList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    //vertexBuffer.LoadData(device.GetDevice(), verteices);
}


