#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"MWindow.h"
#include"MVulkanEngine.h"
#include <stdexcept>

#include "MVulkanShader.h"

float verteices[] = {
    0.f, 1.f, 0.f,
    -1.f, -1.f, 0.f,
    1.f, -1.f, 0.f
};


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

void MVulkanEngine::drawFrame()
{
    VK_CHECK_RESULT(vkWaitForFences(device.GetDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device.GetDevice(), swapChain.Get(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    //if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    //    recreateSwapChain();
    //    return;
    //}
    //else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    //    throw std::runtime_error("failed to acquire swap chain image!");
    //}

    VK_CHECK_RESULT(vkResetFences(device.GetDevice(), 1, &inFlightFences[currentFrame]));

    //vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    graphicsLists[currentFrame].Reset();
    
    recordCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    //spdlog::info("aaaaaaaaaaaaaaaaaaaaa");
    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue.Get(), 1, &submitInfo, inFlightFences[currentFrame]));
    //spdlog::info("bbbbbbbbbbbbbbbbbbbbb");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain.Get()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue.Get(), &presentInfo);

    //if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    //    framebufferResized = false;
    //    recreateSwapChain();
    //}
    //else if (result != VK_SUCCESS) {
    //    throw std::runtime_error("failed to present swap chain image!");
    //}

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
    createSyncObjects();

    createBufferAndLoadData();
}

void MVulkanEngine::renderLoop()
{
    window->WindowPollEvents();
    drawFrame();

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
    transferQueue.SetQueue(device.GetQueue(QueueType::TRANSFER_QUEUE));
    presentQueue.SetQueue(device.GetQueue(QueueType::PRESENT_QUEUE));
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

    graphicsLists.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        graphicsLists[i].Create(device.GetDevice(), info);
    }

    info.commandPool = commandAllocator.Get(QueueType::TRANSFER_QUEUE);
    transferList.Create(device.GetDevice(), info);

    info.commandPool = commandAllocator.Get(QueueType::PRESENT_QUEUE);
    presentList.Create(device.GetDevice(), info);
}

void MVulkanEngine::createBufferAndLoadData()
{
    BufferCreateInfo info;
    info.size = sizeof(verteices);
    
    transferList.Reset();
    transferList.Begin();
    vertexBuffer.CreateAndLoadData(transferList, device, info, verteices);
    transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    //vertexBuffer.LoadData(device.GetDevice(), verteices);
}

void MVulkanEngine::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device.GetDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void MVulkanEngine::recordCommandBuffer(uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    //graphicsLists[currentFrame].Reset();
    VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsLists[currentFrame].GetBuffer(), &beginInfo));

    VkExtent2D swapChainExtent = swapChain.GetSwapChainExtent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass.Get();
    renderPassInfo.framebuffer = frameBuffers[imageIndex].Get();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(graphicsLists[currentFrame].GetBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(graphicsLists[currentFrame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Get());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(graphicsLists[currentFrame].GetBuffer(), 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(graphicsLists[currentFrame].GetBuffer(), 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { vertexBuffer.GetBuffer()};
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(graphicsLists[currentFrame].GetBuffer(), 0, 1, vertexBuffers, offsets);

    vkCmdDraw(graphicsLists[currentFrame].GetBuffer(), 3, 1, 0, 0);
    //vkCmdDraw(graphicsLists[currentFrame].GetBuffer(), static_cast<uint32_t>(sizeof(verteices)/(3*sizeof(float))), 1, 0, 0);

    vkCmdEndRenderPass(graphicsLists[currentFrame].GetBuffer());

    VK_CHECK_RESULT(vkEndCommandBuffer(graphicsLists[currentFrame].GetBuffer()));
}


