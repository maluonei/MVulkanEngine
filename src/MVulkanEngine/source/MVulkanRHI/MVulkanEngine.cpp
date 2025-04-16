#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"MVulkanRHI/MWindow.hpp"
#include"MVulkanRHI/MVulkanEngine.hpp"
#include "MVulkanRHI/MVulkanCommand.hpp"
#include <stdexcept>
#include "Camera.hpp"
#include "Scene/Scene.hpp"
#include "ComputePass.hpp"

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
    m_window = new MWindow();
}

MVulkanEngine::~MVulkanEngine()
{
    
}

void MVulkanEngine::Init()
{
    m_window->Init(m_windowWidth, m_windowHeight);

    initVulkan();
}

void MVulkanEngine::Clean()
{
    Singleton<TextureManager>::instance().Clean();

    m_placeHolderTexture->Clean();

    m_allocator.Clean();
    
    m_graphicsQueue.Clean();
    m_transferQueue.Clean();
    m_presentQueue.Clean();

    m_generalGraphicList.Clean();
    m_transferList.Clean();
    m_presentList.Clean();
    for (auto list : m_graphicsLists) {
        list.Clean();
    }
    m_commandAllocator.Clean();

    for (auto i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++)
    {
        m_imageAvailableSemaphores[i].Clean();
        m_finalRenderFinishedSemaphores[i].Clean();
        m_inFlightFences[i].Clean();
    }

    m_swapChain.Clean();
    m_window->Clean();

    m_device.Clean();
    vkDestroySurfaceKHR(m_instance.GetInstance(), m_surface, nullptr);
    m_instance.Clean();
}

void MVulkanEngine::SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight)
{
    m_windowWidth = _windowWidth;
    m_windowHeight = _windowHeight;
}

void MVulkanEngine::GenerateMipMap(MVulkanTexture texture)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_device.GetPhysicalDevice(), texture.GetImageInfo().format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    m_generalGraphicList.Reset();
    m_generalGraphicList.Begin();

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

        m_generalGraphicList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

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
    
        m_generalGraphicList.BlitImage(
            texture.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            texture.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            blits,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
        m_generalGraphicList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }
    //
    barrier.baseMipLevel = texture.GetImageInfo().mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    m_generalGraphicList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
   
    m_generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_generalGraphicList.GetBuffer();

    m_graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_graphicsQueue.WaitForQueueComplete();
}


MVulkanCommandQueue& MVulkanEngine::GetCommandQueue(MQueueType type)
{
    switch (type) {
    case MQueueType::GRAPHICS:
        return m_graphicsQueue;
    case MQueueType::TRANSFER:
        return m_transferQueue;
    case MQueueType::PRESENT:
        return m_presentQueue;
    case MQueueType::COMPUTE:
        return m_computeQueue;
    }
}

MGraphicsCommandList& MVulkanEngine::GetCommandList(MQueueType type)
{
    switch (type) {
    case MQueueType::GRAPHICS:
        return m_generalGraphicList;
    case MQueueType::TRANSFER:
        return m_transferList;
    case MQueueType::PRESENT:
        return m_presentList;
    //case MQueueType::COMPUTE:
    //    return m_computeList;
    }
}

MGraphicsCommandList& MVulkanEngine::GetGraphicsList(int i)
{
    return m_graphicsLists[i];
}

//void MVulkanEngine::CreateRenderPass(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<ShaderModule> shader, 
//    std::vector<std::vector<VkImageView>> imageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    renderPass->Create(shader, m_swapChain, m_graphicsQueue, m_generalGraphicList, m_allocator, imageViews, samplers, accelerationStructures);
//}
//
//
//void MVulkanEngine::CreateRenderPass(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<ShaderModule> shader, std::vector<uint32_t> storageBufferSizes,
//    std::vector<std::vector<VkImageView>> imageViews, std::vector<std::vector<VkImageView>> storageImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    renderPass->Create(shader, m_swapChain, m_graphicsQueue, m_generalGraphicList, m_allocator, storageBufferSizes, imageViews, storageImageViews, samplers, accelerationStructures);
//}
//
//void MVulkanEngine::CreateRenderPass(std::shared_ptr<RenderPass> renderPass,
//    std::shared_ptr<ShaderModule> shader,
//    std::vector<StorageBuffer> storageBuffers,
//    std::vector<std::vector<VkImageView>> imageViews,
//    std::vector<std::vector<VkImageView>> storageImageViews,
//    std::vector<VkSampler> samplers,
//    std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    renderPass->Create(shader, m_swapChain, m_graphicsQueue, m_generalGraphicList, m_allocator, storageBuffers, imageViews, storageImageViews, samplers, accelerationStructures);
//}
//
//void MVulkanEngine::CreateRenderPass(std::shared_ptr<RenderPass> renderPass,
//    std::shared_ptr<MeshShaderModule> shader,
//    std::vector<StorageBuffer> storageBuffers,
//    std::vector<std::vector<VkImageView>> imageViews,
//    std::vector<std::vector<VkImageView>> storageImageViews,
//    std::vector<VkSampler> samplers,
//    std::vector<VkAccelerationStructureKHR> accelerationStructures) 
//{
//    renderPass->Create(shader, m_swapChain, m_graphicsQueue, m_generalGraphicList, m_allocator, storageBuffers, imageViews, storageImageViews, samplers, accelerationStructures);
//}

//void MVulkanEngine::CreateRenderPass(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<ShaderModule> shader, std::vector<PassResources> resources)
//{
//    renderPass->Create(shader, m_swapChain, m_graphicsQueue, m_generalGraphicList, m_allocator, resources);
//}

void MVulkanEngine::CreateRenderPass(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<ShaderModule> shader)
{
    if (renderPass->GetRenderPassCreateInfo().dynamicRender) {
        renderPass->CreateDynamic(shader, m_swapChain, m_graphicsQueue, m_generalGraphicList, m_allocator);
    }
    else {
        renderPass->Create(shader, m_swapChain, m_graphicsQueue, m_generalGraphicList, m_allocator);
    }
}

void MVulkanEngine::CreateComputePass(std::shared_ptr<ComputePass> computePass, std::shared_ptr<ComputeShaderModule> shader)
{
    computePass->Create(shader, m_allocator);
}

//void MVulkanEngine::CreateDynamicRenderPass(std::shared_ptr<DynamicRenderPass> renderPass, std::shared_ptr<ShaderModule> shader, std::vector<StorageBuffer> storageBuffers, std::vector<std::vector<VkImageView>> imageViews, std::vector<std::vector<VkImageView>> storageImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    renderPass->Create(shader, m_swapChain, m_graphicsQueue, m_generalGraphicList, m_allocator, storageBuffers, imageViews, storageImageViews, samplers, accelerationStructures);
//}

//void MVulkanEngine::CreateComputePass(std::shared_ptr<ComputePass> computePass, std::shared_ptr<ComputeShaderModule> shader, 
//    std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos,
//    std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    computePass->Create(shader, m_allocator, storageBufferSizes, storageImageCreateInfos, seperateImageViews, samplers, accelerationStructures);
//}
//
//void MVulkanEngine::CreateComputePass(std::shared_ptr<ComputePass> computePass, std::shared_ptr<ComputeShaderModule> shader,
//    std::vector<uint32_t> storageBufferSizes,
//    std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<std::vector<VkImageView>> storageImageViews,
//    std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures) 
//{
//    computePass->Create(shader, m_allocator, storageBufferSizes, seperateImageViews, storageImageViews, samplers, accelerationStructures);
//}
//
//void MVulkanEngine::CreateComputePass(std::shared_ptr<ComputePass> computePass,
//    std::shared_ptr<ComputeShaderModule> shader,
//    std::vector<StorageBuffer> storageBuffers,
//    std::vector<std::vector<VkImageView>> seperateImageViews,
//    std::vector<std::vector<VkImageView>> storageImageViews,
//    std::vector<VkSampler> samplers,
//    std::vector<VkAccelerationStructureKHR> accelerationStructures) 
//{
//    computePass->Create(shader, m_allocator, storageBuffers, seperateImageViews, storageImageViews, samplers, accelerationStructures);
//}

//void MVulkanEngine::CreateComputePass(std::shared_ptr<ComputePass> computePass, std::shared_ptr<ComputeShaderModule> shader,
//    std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<VkImageView>> storageImageViews,
//    std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures) 
//{
//    computePass->Create(shader, m_allocator, storageBufferSizes, storageImageViews, seperateImageViews, samplers, accelerationStructures);
//}

bool MVulkanEngine::WindowShouldClose() const
{
    return m_window->WindowShouldClose();
}

void MVulkanEngine::PollWindowEvents()
{
    m_window->WindowPollEvents();
}

VkResult MVulkanEngine::AcquireNextSwapchainImage(uint32_t& imageIndex, uint32_t currentFrame)
{
    return m_swapChain.AcquireNextImage(m_imageAvailableSemaphores[currentFrame].GetSemaphore(), VK_NULL_HANDLE, &imageIndex);
}

void MVulkanEngine::SubmitGraphicsCommands(
    uint32_t imageIndex, uint32_t currentFrame)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores3[] = { m_imageAvailableSemaphores[currentFrame].GetSemaphore() };
    VkPipelineStageFlags waitStages3[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores3;
    submitInfo.pWaitDstStageMask = waitStages3;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores3[] = { m_finalRenderFinishedSemaphores[currentFrame].GetSemaphore() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores3;

    m_graphicsQueue.SubmitCommands(1, &submitInfo, m_inFlightFences[currentFrame].GetFence());
    m_graphicsQueue.WaitForQueueComplete();

    VkSemaphore transferSignalSemaphores3[] = { m_finalRenderFinishedSemaphores[currentFrame].GetSemaphore() };

    vkDeviceWaitIdle(m_device.GetDevice());
    //present(m_swapChain.GetPtr(), currentFrame, &imageIndex, recreateSwapchain);
    //present(m_swapChain.GetPtr(), transferSignalSemaphores3, &imageIndex, recreateSwapchain);
}

void MVulkanEngine::RecordCommandBuffer(uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, 
    uint32_t currentFrame, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount, 
    bool flipY)
{
    recordCommandBuffer(frameIndex, renderPass, m_graphicsLists[currentFrame], vertexBuffer, indexBuffer, indirectBuffer, indirectCount, flipY);
}

void MVulkanEngine::RecordCommandBuffer(uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, 
    uint32_t currentFrame, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount, 
    std::string eventName, bool flipY)
{
    recordCommandBuffer(frameIndex, renderPass, m_graphicsLists[currentFrame], vertexBuffer, indexBuffer, indirectBuffer, indirectCount, eventName, flipY);
}

void MVulkanEngine::RecordCommandBuffer(
    uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame,
    std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<StorageBuffer> indirectBuffer, 
    uint32_t indirectBufferOffset, uint32_t indirectCount,
    bool flipY) {
    recordCommandBuffer(frameIndex, renderPass, m_graphicsLists[currentFrame], vertexBuffer, indexBuffer, indirectBuffer, indirectBufferOffset, indirectCount, flipY);
}

void MVulkanEngine::RecordCommandBuffer(
    uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame, 
    std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<StorageBuffer> indirectBuffer, uint32_t indirectBufferOffset, uint32_t indirectCount, 
    std::string eventName, bool flipY)
{
    recordCommandBuffer(frameIndex, renderPass, m_graphicsLists[currentFrame], vertexBuffer, indexBuffer, indirectBuffer, indirectBufferOffset, indirectCount, eventName, flipY);
}

void MVulkanEngine::RecordCommandBuffer(uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount, bool flipY)
{
    recordCommandBuffer(frameIndex, renderPass, m_generalGraphicList, vertexBuffer, indexBuffer, indirectBuffer, indirectCount, flipY);
}

void MVulkanEngine::RecordCommandBuffer(
    uint32_t frameIndex,
    std::shared_ptr<RenderPass> renderPass,
    uint32_t currentFrame,
    RenderingInfo& renderingInfo,
    std::shared_ptr<Buffer> vertexBuffer,
    std::shared_ptr<Buffer> indexBuffer,
    std::shared_ptr<Buffer> indirectBuffer,
    uint32_t indirectCount,
    std::string eventName, bool flipY) {
    recordCommandBuffer(frameIndex, renderPass, currentFrame, renderingInfo, vertexBuffer, indexBuffer, indirectBuffer, indirectCount, eventName, flipY);
}

void MVulkanEngine::RecordCommandBuffer2(
    uint32_t frameIndex, 
    std::shared_ptr<DynamicRenderPass> renderPass, 
    uint32_t currentFrame,
    const RenderingInfo& renderingInfo,
    std::shared_ptr<Buffer> vertexBuffer, 
    std::shared_ptr<Buffer> indexBuffer,
    std::shared_ptr<Buffer> indirectBuffer,
    uint32_t indirectCount,
    std::string eventName, bool flipY)
{
    recordCommandBuffer2(frameIndex, renderPass, m_graphicsLists[currentFrame], renderingInfo, vertexBuffer, indexBuffer, indirectBuffer, indirectCount, eventName, flipY);
}

void MVulkanEngine::RecordComputeCommandBuffer(std::shared_ptr<ComputePass> computePass,
    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) 
{
    recordComputeCommandBuffer(computePass, groupCountX, groupCountY, groupCountZ);
}

void MVulkanEngine::RecordComputeCommandBuffer(std::shared_ptr<ComputePass> computePass,
    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, std::string eventName)
{
    recordComputeCommandBuffer(computePass, groupCountX, groupCountY, groupCountZ, eventName);
}

void MVulkanEngine::RecordMeshShaderCommandBuffer(
    uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame, 
    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, std::string eventName, bool flipY)
{
    recordMeshShaderCommandBuffer(frameIndex, renderPass, m_graphicsLists[currentFrame], groupCountX, groupCountY, groupCountZ, eventName, flipY);
}


void MVulkanEngine::SetCamera(std::shared_ptr<Camera> camera)
{
    m_camera = camera;
}

bool MVulkanEngine::RecreateSwapchain()
{
    return m_swapChain.Recreate();
}

void MVulkanEngine::RecreateRenderPassFrameBuffer(std::shared_ptr<RenderPass> renderPass)
{
    renderPass->RecreateFrameBuffers(m_swapChain, m_transferQueue, m_transferList, m_swapChain.GetSwapChainExtent());
}

std::shared_ptr<MVulkanTexture> MVulkanEngine::GetPlaceHolderTexture()
{
    return m_placeHolderTexture;
}

std::shared_ptr<StorageBuffer> MVulkanEngine::CreateStorageBuffer(BufferCreateInfo info, void* data)
{
    std::shared_ptr<StorageBuffer> buffer = std::make_shared<StorageBuffer>(info.usage);
    //info.arrayLength = info.arrayLength;
    ////info.size = bindings[binding].size;
    //info.size = info.size;
    if (data == nullptr) {
        buffer->Create(m_device, info);
        return buffer;
    }

    m_transferList.Reset();
    m_transferList.Begin();
    buffer->CreateAndLoadData(&m_transferList, m_device, info, data);

    m_transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_transferList.GetBuffer();

    m_transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_transferQueue.WaitForQueueComplete();

    return buffer;
}

void MVulkanEngine::TransitionImageLayout(std::vector<MVulkanImageMemoryBarrier> barriers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
    m_generalGraphicList.Reset();
    m_generalGraphicList.Begin();

    m_generalGraphicList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    m_generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_generalGraphicList.GetBuffer();

    m_graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::TransitionImageLayout(MVulkanImageMemoryBarrier barrier, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
    m_generalGraphicList.Reset();
    m_generalGraphicList.Begin();

    m_generalGraphicList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    m_generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_generalGraphicList.GetBuffer();

    m_graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::TransitionImageLayout2(int commandListId, std::vector<MVulkanImageMemoryBarrier> barriers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
    auto graphicsList = m_graphicsLists[commandListId];
    graphicsList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}

void MVulkanEngine::TransitionImageLayout2(MGraphicsCommandList list, std::vector<MVulkanImageMemoryBarrier> barriers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
    list.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
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
    m_instance.Create();
    m_instance.SetupDebugMessenger();
}

void MVulkanEngine::createDevice()
{
    m_device.Create(m_instance.GetInstance(), m_surface);
}

void MVulkanEngine::createSurface()
{
    if (glfwCreateWindowSurface(m_instance.GetInstance(), m_window->GetWindow(), nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void MVulkanEngine::createSwapChain()
{
    m_swapChain.Create(m_device, m_window->GetWindow(), m_surface);
}

void MVulkanEngine::transitionSwapchainImageFormat()
{
    m_transferList.Reset();
    m_transferList.Begin();

    MVulkanImageMemoryBarrier barrier{};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    std::vector<VkImage> images = m_swapChain.GetSwapChainImages();
    std::vector<MVulkanImageMemoryBarrier> barriers;
    for (auto i = 0; i < images.size(); i++) {
        barrier.image = images[i];
        barriers.push_back(barrier);
    }

    m_transferList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    m_transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_transferList.GetBuffer();

    m_transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_transferQueue.WaitForQueueComplete();
}

void MVulkanEngine::transitionFramebufferImageLayout()
{
    m_transferList.Reset();
    m_transferList.Begin();

    MVulkanImageMemoryBarrier barrier{};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    std::vector<MVulkanImageMemoryBarrier> barriers;

    m_transferList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    m_transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_transferList.GetBuffer();

    m_transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_transferQueue.WaitForQueueComplete();
}

void MVulkanEngine::createDescriptorSetAllocator()
{
    m_allocator.Create(m_device.GetDevice());
}

void MVulkanEngine::createCommandQueue()
{
    m_graphicsQueue.SetQueue(m_device.GetQueue(QueueType::GRAPHICS_QUEUE));
    m_transferQueue.SetQueue(m_device.GetQueue(QueueType::TRANSFER_QUEUE));
    m_presentQueue.SetQueue(m_device.GetQueue(QueueType::PRESENT_QUEUE));
    m_computeQueue.SetQueue(m_device.GetQueue(QueueType::COMPUTE_QUEUE));
}

void MVulkanEngine::createCommandAllocator()
{
    m_commandAllocator.Create(m_device);
}

void MVulkanEngine::createCommandList()
{
    MVulkanCommandListCreateInfo info;
    info.commandPool = m_commandAllocator.Get(QueueType::GRAPHICS_QUEUE);
    info.commandBufferCount = 1;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    m_graphicsLists.resize(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    for (int i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++) {
        m_graphicsLists[i].Create(m_device.GetDevice(), info);
    }

    m_generalGraphicList.Create(m_device.GetDevice(), info);

    info.commandPool = m_commandAllocator.Get(QueueType::TRANSFER_QUEUE);
    m_transferList.Create(m_device.GetDevice(), info);

    info.commandPool = m_commandAllocator.Get(QueueType::PRESENT_QUEUE);
    m_presentList.Create(m_device.GetDevice(), info);

    if (m_device.SupportRayTracing()) {
        info.commandPool = m_commandAllocator.Get(QueueType::GRAPHICS_QUEUE);
        m_raytracingList.Create(m_device.GetDevice(), info);
    }

    info.commandPool = m_commandAllocator.Get(QueueType::COMPUTE_QUEUE);
    m_computeList.Create(m_device.GetDevice(), info);
}

void MVulkanEngine::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    m_finalRenderFinishedSemaphores.resize(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    m_inFlightFences.resize(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());

    for (size_t i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++) {
        m_imageAvailableSemaphores[i].Create(m_device.GetDevice());
        m_finalRenderFinishedSemaphores[i].Create(m_device.GetDevice());
        m_inFlightFences[i].Create(m_device.GetDevice());
    }
}

void MVulkanEngine::renderPass(uint32_t currentFrame, uint32_t imageIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) {
    m_graphicsLists[currentFrame].Reset();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { waitSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores[] = { signalSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    m_graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
    m_graphicsQueue.WaitForQueueComplete();
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
    m_generalGraphicList.Reset();
    m_generalGraphicList.Begin();

    m_generalGraphicList.TransitionImageLayout(barrier, srcStage, dstStage);

    m_generalGraphicList.End();

    VkSubmitInfo submitInfo = generateSubmitInfo(&m_generalGraphicList.GetBuffer(), waitSemaphores, waitSemaphoreStage, signalSemaphores);

    m_graphicsQueue.SubmitCommands(1, &submitInfo, fence);
    m_graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::transitionImageLayouts(
    std::vector<MVulkanImageMemoryBarrier> barriers, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage,
    VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitSemaphoreStage, VkSemaphore* signalSemaphores, VkFence fence
)
{
    m_generalGraphicList.Reset();
    m_generalGraphicList.Begin();

    m_generalGraphicList.TransitionImageLayout(barriers, srcStage, dstStage);

    m_generalGraphicList.End();

    VkSubmitInfo submitInfo = generateSubmitInfo(&m_generalGraphicList.GetBuffer(), waitSemaphores, waitSemaphoreStage, signalSemaphores);

    m_graphicsQueue.SubmitCommands(1, &submitInfo, fence);
    m_graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::Present(uint32_t currentFrame, const uint32_t* imageIndex, std::function<void()> recreateSwapchain)
{
    VkSemaphore waitSemaphore[] = { m_finalRenderFinishedSemaphores[currentFrame].GetSemaphore() };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphore;

    presentInfo.swapchainCount = 1;
    auto swapchain = m_swapChain.Get();
    presentInfo.pSwapchains = &swapchain;

    presentInfo.pImageIndices = imageIndex;

    VkResult result = m_presentQueue.Present(&presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window->GetWindowResized()) {
        m_window->SetWindowResized(false);
        recreateSwapchain();
        //RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void MVulkanEngine::CreateBuffer(std::shared_ptr<Buffer> buffer, const void* data, size_t size)
{
    BufferCreateInfo vertexInfo;
    vertexInfo.size = size;

    m_transferList.Reset();
    m_transferList.Begin();
    buffer->CreateAndLoadData(&m_transferList, m_device, vertexInfo, data);

    m_transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_transferList.GetBuffer();

    m_transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_transferQueue.WaitForQueueComplete();
}

void  MVulkanEngine::CreateColorAttachmentImage(
    std::shared_ptr<MVulkanTexture> texture,
    VkExtent2D extent,
    VkFormat format
) {
    //texture = std::make_shared<MVulkanTexture>();
    
    ImageCreateInfo imageInfo;
    ImageViewCreateInfo viewInfo;
    imageInfo.arrayLength = 1;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.width = extent.width;
    imageInfo.height = extent.height;
    imageInfo.format = format;

    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.baseMipLevel = 0;
    viewInfo.levelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.layerCount = 1;

    //CreateImage(texture, imageInfo, viewInfo);
    CreateImage(texture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void  MVulkanEngine::CreateDepthAttachmentImage(
    std::shared_ptr<MVulkanTexture> texture,
    VkExtent2D extent,
    VkFormat format
) {
    //texture = std::make_shared<MVulkanTexture>();

    ImageCreateInfo imageInfo;
    ImageViewCreateInfo viewInfo;
    imageInfo.arrayLength = 1;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.width = extent.width;
    imageInfo.height = extent.height;
    imageInfo.format = format;

    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.flag = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.baseMipLevel = 0;
    viewInfo.levelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.layerCount = 1;

    //CreateImage(texture, imageInfo, viewInfo);
    CreateImage(texture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void MVulkanEngine::CreateImage(std::shared_ptr<MVulkanTexture> texture, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo, VkImageLayout layout)
{
    //texture = std::make_shared<MVulkanTexture>();

    m_generalGraphicList.Reset();
    m_generalGraphicList.Begin();

    texture->Create(&m_generalGraphicList, m_device, imageInfo, viewInfo, layout);

    m_generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_generalGraphicList.GetBuffer();

    m_graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::CreateImage(std::shared_ptr<MVulkanTexture> texture, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo)
{
    //texture = std::make_shared<MVulkanTexture>();

    //m_generalGraphicList.Reset();
    //m_generalGraphicList.Begin();

    texture->Create(m_device, imageInfo, viewInfo);

    //m_generalGraphicList.End();
    //
    //VkSubmitInfo submitInfo{};
    //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //submitInfo.commandBufferCount = 1;
    //submitInfo.pCommandBuffers = &m_generalGraphicList.GetBuffer();
    //
    //m_graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    //m_graphicsQueue.WaitForQueueComplete();
}

//void MVulkanEngine::present(
//    VkSwapchainKHR* swapChains, VkSemaphore* waitSemaphore, 
//    const uint32_t* imageIndex, std::function<void()> recreateSwapchain)
//{
//    VkPresentInfoKHR presentInfo{};
//    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//
//    presentInfo.waitSemaphoreCount = 1;
//    presentInfo.pWaitSemaphores = waitSemaphore;
//
//    presentInfo.swapchainCount = 1;
//    presentInfo.pSwapchains = swapChains;
//
//    presentInfo.pImageIndices = imageIndex;
//
//    VkResult result = m_presentQueue.Present(&presentInfo);
//
//    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window->GetWindowResized()) {
//        m_window->SetWindowResized(false);
//        recreateSwapchain();
//        //RecreateSwapChain();
//    }
//    else if (result != VK_SUCCESS) {
//        throw std::runtime_error("failed to present swap chain image!");
//    }
//}

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

    m_generalGraphicList.Reset();
    m_generalGraphicList.Begin();

    m_placeHolderTexture = std::make_shared<MVulkanTexture>();
    m_placeHolderTexture->Create(&m_generalGraphicList, m_device, imageinfo, viewInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    m_generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_generalGraphicList.GetBuffer();

    m_graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    m_graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::InitSingleton()
{
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

    renderPass->LoadCBV(m_device.GetUniformBufferOffsetAlignment());
    //renderPass->LoadStorageBuffer(m_device.GetUniformBufferOffsetAlignment());

    auto descriptorSets = renderPass->GetDescriptorSets(imageIndex);
    commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), descriptorSets);
	//commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);


    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    commandList.BindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandList.BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);

    commandList.DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, sizeof(VkDrawIndexedIndirectCommand));

    commandList.EndRenderPass();
}

void MVulkanEngine::recordCommandBuffer(uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList, 
    std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount, 
    std::string eventName, bool flipY)
{
    commandList.BeginDebugLabel(eventName);

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

    renderPass->LoadCBV(m_device.GetUniformBufferOffsetAlignment());
    //renderPass->LoadStorageBuffer(m_device.GetUniformBufferOffsetAlignment());

    //auto descriptorSet = renderPass->GetDescriptorSet(imageIndex).Get();
    //commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);
    auto descriptorSets = renderPass->GetDescriptorSets(imageIndex);
    commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), descriptorSets);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    commandList.BindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandList.BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);

    commandList.DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, sizeof(VkDrawIndexedIndirectCommand));

    commandList.EndRenderPass();

    commandList.EndDebugLabel();
}

void MVulkanEngine::recordCommandBuffer(
    uint32_t imageIndex, 
    std::shared_ptr<RenderPass> renderPass,
    MGraphicsCommandList commandList, 
    std::shared_ptr<Buffer> vertexBuffer,
    std::shared_ptr<Buffer> indexBuffer, 
    std::shared_ptr<StorageBuffer> indirectBuffer,
    uint32_t indirectBufferOffset,
    uint32_t indirectCount,
    bool flipY)
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

    renderPass->LoadCBV(m_device.GetUniformBufferOffsetAlignment());
    //renderPass->LoadStorageBuffer(m_device.GetUniformBufferOffsetAlignment());

    //auto descriptorSet = renderPass->GetDescriptorSet(imageIndex).Get();
    //commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);
    auto descriptorSets = renderPass->GetDescriptorSets(imageIndex);
    commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), descriptorSets);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    commandList.BindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandList.BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);

    commandList.DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, indirectBufferOffset);

    commandList.EndRenderPass();
}

void MVulkanEngine::recordCommandBuffer(uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<StorageBuffer> indirectBuffer, uint32_t indirectBufferOffset, uint32_t indirectCount, std::string eventName, bool flipY)
{
    commandList.BeginDebugLabel(eventName);

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

    renderPass->LoadCBV(m_device.GetUniformBufferOffsetAlignment());
    //renderPass->LoadStorageBuffer(m_device.GetUniformBufferOffsetAlignment());

    //auto descriptorSet = renderPass->GetDescriptorSet(imageIndex).Get();
    //commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);
    auto descriptorSets = renderPass->GetDescriptorSets(imageIndex);
    commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), descriptorSets);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    commandList.BindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandList.BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);

    commandList.DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, indirectBufferOffset);

    commandList.EndRenderPass();

    commandList.EndDebugLabel();
}

void MVulkanEngine::recordCommandBuffer(uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, uint32_t _currentFrame, RenderingInfo& renderingInfo, std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount, std::string eventName, bool flipY)
{
    auto& commandList = m_graphicsLists[_currentFrame];

    commandList.BeginDebugLabel(eventName);

    auto extent = renderingInfo.extent;
    auto offset = renderingInfo.offset;

    prepareRenderingInfo(imageIndex, renderingInfo, _currentFrame);
    renderPass->PrepareResourcesForShaderRead(_currentFrame);

    commandList.BeginRendering(renderingInfo);

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

    //renderPass->LoadCBV(m_device.GetUniformBufferOffsetAlignment());

    auto descriptorSets = renderPass->GetDescriptorSets(imageIndex);
    commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), descriptorSets);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    commandList.BindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandList.BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);

    commandList.DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, sizeof(VkDrawIndexedIndirectCommand));

    commandList.EndRendering();

    commandList.EndDebugLabel();

    //use swapchain Image
    if (renderingInfo.colorAttachments[0].texture == nullptr) {
        swapchainImage2Present(imageIndex, _currentFrame);
    }
}

void MVulkanEngine::prepareRenderingInfo(uint32_t imageIndex, RenderingInfo& renderingInfo, uint32_t currentFrame)
{
    TextureState state;
    state.m_stage = ShaderStageFlagBits::FRAGMENT;

    for (auto& attachment : renderingInfo.colorAttachments) {
        if (attachment.texture != nullptr) {
            auto& texture = attachment.texture;
            state.m_state = ETextureState::ColorAttachment;
            texture->TransferTextureState(currentFrame, state);
        }
        else {
            std::vector<MVulkanImageMemoryBarrier> barriers;
            {
                MVulkanImageMemoryBarrier barrier{};
                barrier.image = Singleton<MVulkanEngine>::instance().GetSwapchain().GetImage(imageIndex);
                barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.baseArrayLayer = 0;
                barrier.layerCount = 1;
                barrier.levelCount = 1;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = 0;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                barriers.push_back(barrier);
            }
            Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        }
    }

    {
        if (renderingInfo.depthAttachment.texture != nullptr) {
            auto& texture = renderingInfo.depthAttachment.texture;
            state.m_state = ETextureState::DepthAttachment;
            texture->TransferTextureState(currentFrame, state);
        }
    }
}

void MVulkanEngine::swapchainImage2Present(uint32_t imageIndex, uint32_t currentFrame)
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = Singleton<MVulkanEngine>::instance().GetSwapchain().GetImage(imageIndex);
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barriers.push_back(barrier);
    }

    Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_NONE);
}

void MVulkanEngine::recordCommandBuffer2(
    uint32_t imageIndex, 
    std::shared_ptr<DynamicRenderPass> renderPass, 
    MGraphicsCommandList commandList, 
    const RenderingInfo& renderingInfo,
    std::shared_ptr<Buffer> vertexBuffer, 
    std::shared_ptr<Buffer> indexBuffer, 
    std::shared_ptr<Buffer> indirectBuffer,
    uint32_t indirectCount,
    std::string eventName, bool flipY)
{
    commandList.BeginDebugLabel(eventName);

    auto extent = renderingInfo.extent;
    auto offset = renderingInfo.offset;

    commandList.BeginRendering(renderingInfo);

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

    renderPass->LoadCBV(m_device.GetUniformBufferOffsetAlignment());
    //renderPass->LoadStorageBuffer(m_device.GetUniformBufferOffsetAlignment());

    auto descriptorSet = renderPass->GetDescriptorSet(imageIndex).Get();
    commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    commandList.BindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandList.BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);

    commandList.DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, sizeof(VkDrawIndexedIndirectCommand));

    commandList.EndRendering();

    commandList.EndDebugLabel();
    //commandList.EndRenderPass();
}

void MVulkanEngine::recordComputeCommandBuffer(std::shared_ptr<ComputePass> computePass,
    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    m_computeList.BindPipeline(computePass->GetPipeline().Get());

    computePass->LoadConstantBuffer(m_device.GetUniformBufferOffsetAlignment());
    //computePass->LoadStorageBuffer(m_device.GetUniformBufferOffsetAlignment());

    auto descriptorSets = computePass->GetDescriptorSets();
    m_computeList.BindDescriptorSet(computePass->GetPipeline().GetLayout(), descriptorSets);

    m_computeList.Dispatch(groupCountX, groupCountY, groupCountZ);
}

void MVulkanEngine::recordComputeCommandBuffer(std::shared_ptr<ComputePass> computePass, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, std::string eventName)
{
    m_computeList.BeginDebugLabel(eventName);

    computePass->PrepareResourcesForShaderRead();

    m_computeList.BindPipeline(computePass->GetPipeline().Get());

    computePass->LoadConstantBuffer(m_device.GetUniformBufferOffsetAlignment());
    //computePass->LoadStorageBuffer(m_device.GetUniformBufferOffsetAlignment());

    auto descriptorSets = computePass->GetDescriptorSets();
    m_computeList.BindDescriptorSet(computePass->GetPipeline().GetLayout(), descriptorSets);

    m_computeList.Dispatch(groupCountX, groupCountY, groupCountZ);

    m_computeList.EndDebugLabel();
}

void MVulkanEngine::recordMeshShaderCommandBuffer(uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, std::string eventName, bool flipY)
{
    commandList.BeginDebugLabel(eventName);

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

    //renderPass->LoadCBV(m_device.GetUniformBufferOffsetAlignment());
    //renderPass->LoadStorageBuffer(m_device.GetUniformBufferOffsetAlignment());

    //auto descriptorSet = renderPass->GetDescriptorSet(imageIndex).Get();
    //commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);
    auto descriptorSets = renderPass->GetDescriptorSets(imageIndex);
    commandList.BindDescriptorSet(renderPass->GetPipeline().GetLayout(), descriptorSets);

    //VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    //VkDeviceSize offsets[] = { 0 };
    //
    //commandList.BindVertexBuffers(0, 1, vertexBuffers, offsets);
    //commandList.BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);
    //
    //commandList.DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, indirectBufferOffset);
    
    commandList.DrawMeshTask(groupCountX, groupCountY, groupCountZ);

    commandList.EndRenderPass();

    commandList.EndDebugLabel();
}
