#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"MVulkanRHI/MWindow.hpp"
#include"MVulkanRHI/MVulkanEngine.hpp"
#include "MVulkanRHI/MVulkanCommand.hpp"
#include <stdexcept>
#include "Camera.hpp"
#include "Scene/Scene.hpp"

#include "MVulkanRHI/MVulkanShader.hpp"
#include "RenderPass.hpp"
#include "Managers/GlobalConfig.hpp"
#include "Managers/InputManager.hpp"
#include "Scene/SceneLoader.hpp"
#include "Managers/TextureManager.hpp"
#include "Scene/Light/DirectionalLight.hpp"
#include "Util.hpp"

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

    commandAllocator.Clean(device.GetDevice());

    device.Clean();
    vkDestroySurfaceKHR(instance.GetInstance(), surface, nullptr);
    instance.Clean();
}

void MVulkanEngine::SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight)
{
    windowWidth = _windowWidth;
    windowHeight = _windowHeight;
}

void MVulkanEngine::GenerateMipMap(MVulkanTexture texture)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), texture.GetImageInfo().format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    generalGraphicList.Reset();
    generalGraphicList.Begin();

    MVulkanImageMemoryBarrier barrier{};
    barrier.image = texture.GetImage();
    barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.baseArrayLayer = 0;
    barrier.layerCount = 1;
    barrier.levelCount = 1;
    
    int32_t mipWidth = texture.GetImageInfo().width;
    int32_t mipHeight = texture.GetImageInfo().height;

    //spdlog::info("texture.GetImageInfo().mipLevels: {}", texture.GetImageInfo().mipLevels);
    for (uint32_t i = 1; i < texture.GetImageInfo().mipLevels; i++) {
        barrier.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        generalGraphicList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        std::vector<VkImageBlit> blits(1);
        blits[0] = VkImageBlit();
        blits[0].srcOffsets[0] = { 0, 0, 0 };
        blits[0].srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blits[0].srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blits[0].srcSubresource.mipLevel = i - 1;
        blits[0].srcSubresource.baseArrayLayer = 0;
        blits[0].srcSubresource.layerCount = 1;
        blits[0].dstOffsets[0] = { 0, 0, 0 };
        blits[0].dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blits[0].dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blits[0].dstSubresource.mipLevel = i;
        blits[0].dstSubresource.baseArrayLayer = 0;
        blits[0].dstSubresource.layerCount = 1;
    
        generalGraphicList.BlitImage(
            texture.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            texture.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            blits,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
        generalGraphicList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }
    //
    barrier.baseMipLevel = texture.GetImageInfo().mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    generalGraphicList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
   
    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();
}

MVulkanCommandQueue MVulkanEngine::GetCommandQueue(MQueueType type)
{
    switch (type) {
    case MQueueType::GRAPHICS:
        return graphicsQueue;
    case MQueueType::TRANSFER:
        return transferQueue;
    case MQueueType::PRESENT:
        return presentQueue;
    }
}

MGraphicsCommandList MVulkanEngine::GetCommandList(MQueueType type)
{
    switch (type) {
    case MQueueType::GRAPHICS:
        return generalGraphicList;
    case MQueueType::TRANSFER:
        return transferList;
    case MQueueType::PRESENT:
        return presentList;
    }
}

MGraphicsCommandList MVulkanEngine::GetGraphicsList(int i)
{
    return graphicsLists[i];
}

void MVulkanEngine::CreateRenderPass(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<ShaderModule> shader, std::vector<std::vector<VkImageView>> imageViews, std::vector<VkSampler> samplers)
{
    renderPass->Create(shader, swapChain, graphicsQueue, generalGraphicList, allocator, imageViews, samplers);
}

bool MVulkanEngine::WindowShouldClose() const
{
    return window->WindowShouldClose(); 
}

void MVulkanEngine::PollWindowEvents()
{
    window->WindowPollEvents();
}

VkResult MVulkanEngine::AcquireNextSwapchainImage(uint32_t& imageIndex, uint32_t currentFrame)
{
    return swapChain.AcquireNextImage(device.GetDevice(), imageAvailableSemaphores[currentFrame].GetSemaphore(), VK_NULL_HANDLE, &imageIndex);
}

void MVulkanEngine::SubmitCommandsAndPresent(
    uint32_t imageIndex, uint32_t currentFrame, std::function<void()> recreateSwapchain)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores3[] = { imageAvailableSemaphores[currentFrame].GetSemaphore() };
    VkPipelineStageFlags waitStages3[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores3;
    submitInfo.pWaitDstStageMask = waitStages3;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores3[] = { finalRenderFinishedSemaphores[currentFrame].GetSemaphore() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores3;

    graphicsQueue.SubmitCommands(1, &submitInfo, inFlightFences[currentFrame].GetFence());
    graphicsQueue.WaitForQueueComplete();

    VkSemaphore transferSignalSemaphores3[] = { finalRenderFinishedSemaphores[currentFrame].GetSemaphore() };

    vkDeviceWaitIdle(device.GetDevice());
    present(swapChain.GetPtr(), transferSignalSemaphores3, &imageIndex, recreateSwapchain);
}

void MVulkanEngine::RecordCommandBuffer(uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount, bool flipY)
{
    recordCommandBuffer(frameIndex, renderPass, graphicsLists[currentFrame], vertexBuffer, indexBuffer, indirectBuffer, indirectCount, flipY);
}

void MVulkanEngine::RecordCommandBuffer(uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount, bool flipY)
{
    recordCommandBuffer(frameIndex, renderPass, generalGraphicList, vertexBuffer, indexBuffer, indirectBuffer, indirectCount, flipY);
}

void MVulkanEngine::SetCamera(std::shared_ptr<Camera> camera)
{
    this->camera = camera;
}

bool MVulkanEngine::RecreateSwapchain()
{
    return swapChain.Recreate(device, window->GetWindow(), surface);
}

void MVulkanEngine::RecreateRenderPassFrameBuffer(std::shared_ptr<RenderPass> renderPass)
{
    renderPass->RecreateFrameBuffers(swapChain, transferQueue, transferList, swapChain.GetSwapChainExtent());
}

MVulkanTexture MVulkanEngine::GetPlaceHolderTexture()
{
    return placeHolderTexture;
}


void MVulkanEngine::initVulkan()
{
    createInstance();
    createSurface();
    createDevice();

    createCommandQueue();
    createCommandAllocator();
    createCommandList();
    createSyncObjects();

    createSwapChain();
    
    createDescriptorSetAllocator();

    createPlaceHolderTexture();
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

void MVulkanEngine::transitionSwapchainImageFormat()
{
    transferList.Reset();
    transferList.Begin();

    MVulkanImageMemoryBarrier barrier{};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    std::vector<VkImage> images = swapChain.GetSwapChainImages();
    std::vector<MVulkanImageMemoryBarrier> barriers;
    for (auto i = 0; i < images.size(); i++) {
        barrier.image = images[i];
        barriers.push_back(barrier);
    }

    transferList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();
}

void MVulkanEngine::transitionFramebufferImageLayout()
{
    transferList.Reset();
    transferList.Begin();

    MVulkanImageMemoryBarrier barrier{};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    std::vector<MVulkanImageMemoryBarrier> barriers;

    transferList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();
}

void MVulkanEngine::createDescriptorSetAllocator()
{
    allocator.Create(device.GetDevice());
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

    graphicsLists.resize(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    for (int i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++) {
        graphicsLists[i].Create(device.GetDevice(), info);
    }

    generalGraphicList.Create(device.GetDevice(), info);

    info.commandPool = commandAllocator.Get(QueueType::TRANSFER_QUEUE);
    transferList.Create(device.GetDevice(), info);

    info.commandPool = commandAllocator.Get(QueueType::PRESENT_QUEUE);
    presentList.Create(device.GetDevice(), info);
}

void MVulkanEngine::createSyncObjects()
{
    imageAvailableSemaphores.resize(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    finalRenderFinishedSemaphores.resize(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    inFlightFences.resize(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());

    for (size_t i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++) {
        imageAvailableSemaphores[i].Create(device.GetDevice());
        finalRenderFinishedSemaphores[i].Create(device.GetDevice());
        inFlightFences[i].Create(device.GetDevice());
    }
}

void MVulkanEngine::renderPass(uint32_t currentFrame, uint32_t imageIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) {
    graphicsLists[currentFrame].Reset();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { waitSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores[] = { signalSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
    graphicsQueue.WaitForQueueComplete();
}

VkSubmitInfo MVulkanEngine::generateSubmitInfo(std::vector<VkCommandBuffer> commandBuffers, std::vector<VkSemaphore> waitSemaphores, std::vector<VkPipelineStageFlags> waitSemaphoreStage, std::vector<VkSemaphore> signalSemaphores) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();

    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitSemaphoreStage.data();

    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    return submitInfo;
}

VkSubmitInfo MVulkanEngine::generateSubmitInfo(VkCommandBuffer* commandBuffer, VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitSemaphoreStage, VkSemaphore* signalSemaphores) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = commandBuffer;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitSemaphoreStage;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    return submitInfo;
}

void MVulkanEngine::submitCommand(){

}

void MVulkanEngine::transitionImageLayout(
    MVulkanImageMemoryBarrier barrier, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage,
    VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitSemaphoreStage, VkSemaphore* signalSemaphores, VkFence fence
)
{
    generalGraphicList.Reset();
    generalGraphicList.Begin();

    generalGraphicList.TransitionImageLayout(barrier, srcStage, dstStage);

    generalGraphicList.End();

    VkSubmitInfo submitInfo = generateSubmitInfo(&generalGraphicList.GetBuffer(), waitSemaphores, waitSemaphoreStage, signalSemaphores);

    graphicsQueue.SubmitCommands(1, &submitInfo, fence);
    graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::transitionImageLayouts(
    std::vector<MVulkanImageMemoryBarrier> barriers, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage,
    VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitSemaphoreStage, VkSemaphore* signalSemaphores, VkFence fence
)
{
    generalGraphicList.Reset();
    generalGraphicList.Begin();

    generalGraphicList.TransitionImageLayout(barriers, srcStage, dstStage);

    generalGraphicList.End();

    VkSubmitInfo submitInfo = generateSubmitInfo(&generalGraphicList.GetBuffer(), waitSemaphores, waitSemaphoreStage, signalSemaphores);

    graphicsQueue.SubmitCommands(1, &submitInfo, fence);
    graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::CreateBuffer(std::shared_ptr<Buffer> buffer, const void* data, size_t size)
{
    BufferCreateInfo vertexInfo;
    vertexInfo.size = size;

    transferList.Reset();
    transferList.Begin();
    buffer->CreateAndLoadData(&transferList, device, vertexInfo, data);

    transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();
}

void MVulkanEngine::present(
    VkSwapchainKHR* swapChains, VkSemaphore* waitSemaphore, 
    const uint32_t* imageIndex, std::function<void()> recreateSwapchain)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = imageIndex;

    VkResult result = presentQueue.Present(&presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window->GetWindowResized()) {
        window->SetWindowResized(false);
        recreateSwapchain();
        //RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void MVulkanEngine::createPlaceHolderTexture()
{
    ImageCreateInfo imageinfo{};
    imageinfo.width = 1;
    imageinfo.height = 1;
    imageinfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageinfo.mipLevels = 1;
    imageinfo.arrayLength = 1;
    imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageinfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    ImageViewCreateInfo viewInfo{};
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.levelCount = 1;
    viewInfo.layerCount = 1;

    generalGraphicList.Reset();
    generalGraphicList.Begin();

    placeHolderTexture.Create(&generalGraphicList, device, imageinfo, viewInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::recordCommandBuffer(
    uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList,
    std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount, bool flipY)
{
    VkExtent2D extent = renderPass->GetFrameBuffer(imageIndex).GetExtent2D();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass->GetRenderPass().Get();
    renderPassInfo.framebuffer = renderPass->GetFrameBuffer(imageIndex).Get();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;

    uint32_t attachmentCount = renderPass->GetAttachmentCount();
    std::vector<VkClearValue> clearValues(attachmentCount + 1);
    for (auto i = 0; i < attachmentCount; i++) {
        clearValues[i].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    }
    clearValues[attachmentCount].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    commandList.BeginRenderPass(&renderPassInfo);

    commandList.BindPipeline(renderPass->GetPipeline().Get());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.width = (float)extent.width;
    if (flipY) {
        viewport.y = (float)extent.height;
        viewport.height = -(float)extent.height;
    }
    else {
        viewport.y = 0.f;
        viewport.height = (float)extent.height;
    }
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandList.SetViewport(0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    commandList.SetScissor(0, 1, &scissor);

    renderPass->LoadCBV(device.GetUniformBufferOffsetAlignment());

    auto descriptorSet = renderPass->GetDescriptorSet(imageIndex).Get();
    commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    commandList.BindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandList.BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);

    commandList.DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, sizeof(VkDrawIndexedIndirectCommand));

    commandList.EndRenderPass();
}
