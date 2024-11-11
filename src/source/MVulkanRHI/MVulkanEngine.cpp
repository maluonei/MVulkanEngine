#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"MVulkanRHI/MWindow.hpp"
#include"MVulkanRHI/MVulkanEngine.hpp"
#include <stdexcept>
#include "Camera.hpp"
#include "Scene/Model.hpp"

#include "MVulkanRHI/MVulkanShader.hpp"
#include "RenderPass.hpp"

float verteices[] = {
    0.8f, 0.8f, 0.f,   0.f, 0.f, 1.f,   1.f, 1.f,
    -0.8f, -0.8f, 0.f, 0.f, 0.f, 1.f,   0.f, 0.f,
    0.8f, -0.8f, 0.f,  0.f, 0.f, 1.f,   1.f, 0.f,
    -0.8f, 0.8f, 0.f,  0.f, 0.f, 1.f,   0.f, 1.f,
    1.8f, 1.8f, 0.f,   0.f, 0.f, -1.f,   1.f, 1.f,
    -1.8f, -1.8f, 0.f, 0.f, 0.f, -1.f,   0.f, 0.f,
    1.8f, -1.8f, 0.f,  0.f, 0.f, -1.f,   1.f, 0.f,
    -1.8f, 1.8f, 0.f,  0.f, 0.f, -1.f,   0.f, 1.f,
};

uint32_t indices[] = {
    0, 1, 2,
    0, 3, 1,
    4, 5, 6,
    4, 7, 5
};

float sqad_vertices[] = {
    -1.f, -1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f,
    -1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
    1.f, -1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f
};
uint32_t sqad_indices[] = {
    0, 1, 2,
    2, 1, 3
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

    createCamera();
    initVulkan();
}

void MVulkanEngine::Clean()
{
    window->Clean();

    //SpirvHelper::Finalize();

    commandAllocator.Clean(device.GetDevice());

    for (auto i = 0; i < gbufferFrameBuffers.size(); i++) {
        gbufferFrameBuffers[i].Clean(device.GetDevice());
        finalFrameBuffers[i].Clean(device.GetDevice());
    }

    swapChain.Clean(device.GetDevice());

    gbufferPipeline.Clean(device.GetDevice());
    finalPipeline.Clean(device.GetDevice());
    gbufferRenderPass.Clean(device.GetDevice());
    finalRenderPass.Clean(device.GetDevice());

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
    //VK_CHECK_RESULT(vkWaitForFences(device.GetDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));
    inFlightFences[currentFrame].WaitForSignal(device.GetDevice());

    uint32_t imageIndex;
    VkResult result = swapChain.AcquireNextImage(device.GetDevice(), imageAvailableSemaphores[currentFrame].GetSemaphore(), VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    //VK_CHECK_RESULT(vkResetFences(device.GetDevice(), 1, &inFlightFences[currentFrame]));
    inFlightFences[currentFrame].Reset(device.GetDevice());

    graphicsLists[currentFrame].Reset();
    
    recordGbufferCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores1[] = { imageAvailableSemaphores[currentFrame].GetSemaphore() };
    VkPipelineStageFlags waitStages1[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores1;
    submitInfo.pWaitDstStageMask = waitStages1;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores1[] = { gbufferRenderFinishedSemaphores[currentFrame].GetSemaphore() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores1;

    graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
    graphicsQueue.WaitForQueueComplete();


    std::vector<MVulkanImageMemoryBarrier> barriers;
    for (auto i = 0; i < gbufferFrameBuffers[currentFrame].ColorAttachmentsCount(); i++) {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = gbufferFrameBuffers[currentFrame].GetImage(i);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        barriers.push_back(barrier);
    }

    VkSemaphore transferWaitSemaphores[] = { gbufferRenderFinishedSemaphores[currentFrame].GetSemaphore() };
    VkPipelineStageFlags transferWaitStages1[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore transferSignalSemaphores[] = { gbufferTransferFinishedSemaphores[currentFrame].GetSemaphore() };
    transitionImageLayouts(barriers, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        transferWaitSemaphores, transferWaitStages1,
        transferSignalSemaphores, VK_NULL_HANDLE
    );

    graphicsLists[currentFrame].Reset();
    recordFinalCommandBuffer(imageIndex);

    //VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores2[] = { gbufferTransferFinishedSemaphores[currentFrame].GetSemaphore() };
    VkPipelineStageFlags waitStages2[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores2;
    submitInfo.pWaitDstStageMask = waitStages2;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores2[] = { finalRenderFinishedSemaphores[currentFrame].GetSemaphore() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores2;

    //VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue.Get(), 1, &submitInfo, inFlightFences[currentFrame]));
    //vkQueueWaitIdle(graphicsQueue.Get());
    graphicsQueue.SubmitCommands(1, &submitInfo, inFlightFences[currentFrame].GetFence());
    graphicsQueue.WaitForQueueComplete();
    //done draw gbuffer

    barriers.clear();
    for (auto i = 0; i < gbufferFrameBuffers[currentFrame].ColorAttachmentsCount(); i++) {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = gbufferFrameBuffers[currentFrame].GetImage(i);
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        barriers.push_back(barrier);
    }

    VkSemaphore transferWaitSemaphores2[] = { finalRenderFinishedSemaphores[currentFrame].GetSemaphore() };
    VkPipelineStageFlags transferWaitStages2[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore transferSignalSemaphores2[] = { transferFinishedSemaphores[currentFrame].GetSemaphore() };
    transitionImageLayouts(barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        transferWaitSemaphores2, transferWaitStages2,
        transferSignalSemaphores2, VK_NULL_HANDLE
    );

    present(swapChain.GetPtr(), transferSignalSemaphores2, &imageIndex);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void MVulkanEngine::createGlobalSamplers()
{
    for (auto i = 0; i < 1; i++) {
        MVulkanSampler sampler;
        sampler.Create(device);
        globalSamplers.push_back(sampler);
    }
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

    //VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    transferList.Reset();
    transferList.Begin();

    MVulkanImageMemoryBarrier barrier{};
    barrier.image = texture.GetImage();
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    int32_t mipWidth = texture.GetImageInfo().width;
    int32_t mipHeight = texture.GetImageInfo().height;

    for (uint32_t i = 1; i < texture.GetImageInfo().mipLevels; i++) {
        barrier.baseMipLevel = i - 1;

        transferList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        //vkCmdPipelineBarrier(commandBuffer,
        //    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        //    0, nullptr,
        //    0, nullptr,
        //    1, &barrier);

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
    
        transferList.BlitImage(
            texture.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            texture.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            blits,
            VK_FILTER_LINEAR);
        //vkCmdBlitImage(commandBuffer,
        //    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        //    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //    1, &blit,
        //    VK_FILTER_LINEAR);
    
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
        transferList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        //vkCmdPipelineBarrier(commandBuffer,
        //    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        //    0, nullptr,
        //    0, nullptr,
        //    1, &barrier);
    
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }
    //
    barrier.baseMipLevel = texture.GetImageInfo().mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    transferList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    //vkCmdPipelineBarrier(commandBuffer,
    //    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    //    0, nullptr,
    //    0, nullptr,
    //    1, &barrier);
    

    transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();
    //endSingleTimeCommands(commandBuffer);
}

MVulkanSampler MVulkanEngine::GetGlobalSampler() const
{
    return globalSamplers[0];
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
    //createDepthBuffer();
    createGbufferRenderPass();
    createFinalRenderPass();
    createGbufferFrameBuffers();
    createFrameBuffers();
    createBufferAndLoadData();
    createGbufferPipeline();
    //createPipeline();
    createSquadPipeline();
}

void MVulkanEngine::renderLoop()
{
    window->WindowPollEvents();
    drawFrame();
    //spdlog::info("running");
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

    transitionSwapchainImageFormat();
}

void MVulkanEngine::RecreateSwapChain()
{

    if (swapChain.Recreate(device, window->GetWindow(), surface)) {
        transitionSwapchainImageFormat();

        for (auto frameBuffer : gbufferFrameBuffers) {
            frameBuffer.Clean(device.GetDevice());
        }
        for (auto frameBuffer : finalFrameBuffers) {
            frameBuffer.Clean(device.GetDevice());
        }

        createFrameBuffers();
    }
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

void MVulkanEngine::createGbufferRenderPass()
{
    RenderPassFormatsInfo gbufferFormats;
    gbufferFormats.useResolvedRef = false;
    std::vector<VkFormat> gbufferAttachmentFormats;
    gbufferAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
    gbufferAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
    gbufferFormats.imageFormats = gbufferAttachmentFormats;
    gbufferFormats.depthFormat = device.FindDepthFormat();
    gbufferRenderPass.Create(device, gbufferFormats);
}

void MVulkanEngine::createFinalRenderPass()
{
    RenderPassFormatsInfo finalFormats;
    finalFormats.useResolvedRef = false;
    std::vector<VkFormat> colorAttachmentFormats;
    colorAttachmentFormats.push_back(swapChain.GetSwapChainImageFormat());
    finalFormats.imageFormats = colorAttachmentFormats;
    finalFormats.depthFormat = device.FindDepthFormat();
    finalFormats.resolvedFormat = swapChain.GetSwapChainImageFormat();
    finalFormats.isFinalRenderPass = true;
    finalRenderPass.Create(device, finalFormats);
}

void MVulkanEngine::createRenderPass()
{
    
}

//void MVulkanEngine::resolveDescriptorSet()
//{
//
//}

void MVulkanEngine::createDescriptorSetAllocator()
{
    allocator.Create(device.GetDevice());
}

void MVulkanEngine::createGbufferPipeline()
{
    gbufferShader = std::make_shared<GbufferShader>();
    gbufferShader->Create(device.GetDevice());

    MVulkanShaderReflector vertReflector(gbufferShader->GetVertexShader().GetShader());
    MVulkanShaderReflector fragReflector(gbufferShader->GetFragmentShader().GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();
    //fragReflector.GenerateDescriptorSet();

    ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());

    gbufferDescriptorLayouts.Create(device.GetDevice(), bindings);
    gbufferDescriptorSet.Create(device.GetDevice(), allocator.Get(), gbufferDescriptorLayouts.Get(), MAX_FRAMES_IN_FLIGHT);

    GbufferShader::UniformBufferObject ubo{};
    ubo.Model = glm::mat4(1.f);
    ubo.View = camera->GetViewMatrix();
    ubo.Projection = camera->GetProjMatrix();
    gbufferShader->SetUBO(0, &ubo);
    
    gbufferCbvs0.resize(MAX_FRAMES_IN_FLIGHT);
    BufferCreateInfo _infoUBO0{};
    _infoUBO0.size = gbufferShader->GetBufferSizeBinding(0);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        gbufferCbvs0[i].Create(device, _infoUBO0);
    }

    MVulkanDescriptorSetWrite write;

    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        bufferInfos[i].resize(1);

        bufferInfos[i][0].buffer = gbufferCbvs0[i].GetBuffer();
        bufferInfos[i][0].offset = 0;
        bufferInfos[i][0].range = gbufferShader->GetBufferSizeBinding(0);
    }

    std::vector < std::vector<VkDescriptorImageInfo>> imageInfos(0);

    write.Update(device.GetDevice(), gbufferDescriptorSet.Get(), bufferInfos, imageInfos, MAX_FRAMES_IN_FLIGHT);

    gbufferPipeline.Create(device, 
        gbufferShader->GetVertexShader().GetShaderModule(), gbufferShader->GetFragmentShader().GetShaderModule(), 
        gbufferRenderPass.Get(), info, gbufferDescriptorLayouts.Get(), 2);

    gbufferShader->Clean(device.GetDevice());
}

void MVulkanEngine::createSquadPipeline()
{
    squadPhongShader = std::make_shared<SquadPhongShader>();
    squadPhongShader->Create(device.GetDevice());

    MVulkanShaderReflector vertReflector(squadPhongShader->GetVertexShader().GetShader());
    MVulkanShaderReflector fragReflector(squadPhongShader->GetFragmentShader().GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();

    ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());

    finalDescriptorLayouts.Create(device.GetDevice(), bindings);
    finalDescriptorSet.Create(device.GetDevice(), allocator.Get(), finalDescriptorLayouts.Get(), MAX_FRAMES_IN_FLIGHT);

    createSampler();
    MVulkanDescriptorSetWrite write;

    std::vector < std::vector<VkDescriptorImageInfo>> imageInfos(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        imageInfos[i].resize(2);

        imageInfos[i][0].sampler = sampler.GetSampler();
        imageInfos[i][0].imageView = gbufferFrameBuffers[i].GetImageView(0);
        imageInfos[i][0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageInfos[i][1].sampler = sampler.GetSampler();
        imageInfos[i][1].imageView = gbufferFrameBuffers[i].GetImageView(1);
        imageInfos[i][1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        bufferInfos[i].resize(0);
    }

    write.Update(device.GetDevice(), finalDescriptorSet.Get(), bufferInfos, imageInfos, MAX_FRAMES_IN_FLIGHT);

    finalPipeline.Create(device, squadPhongShader->GetVertexShader().GetShaderModule(), squadPhongShader->GetFragmentShader().GetShaderModule(), finalRenderPass.Get(), info, finalDescriptorLayouts.Get(), 1);

    squadPhongShader->Clean(device.GetDevice());
}

void MVulkanEngine::createPipeline()
{
    //std::string vertPath = "phong.vert.glsl";
    //std::string fragPath = "phong.frag.glsl";
    //
    //MVulkanShader vert(vertPath, ShaderStageFlagBits::VERTEX);
    //MVulkanShader frag(fragPath, ShaderStageFlagBits::FRAGMENT);
    //
    //vert.Create(device.GetDevice());
    //frag.Create(device.GetDevice());
    //
    //MVulkanShaderReflector vertReflector(vert.GetShader());
    //MVulkanShaderReflector fragReflector(frag.GetShader());
    //PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();
    ////fragReflector.GenerateDescriptorSet();
    //
    //ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    //ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    //std::vector<VkDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    //std::vector<VkDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    //bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());
    //
    //layouts.Create(device.GetDevice(), bindings);
    //descriptorSet.Create(device.GetDevice(), allocator.Get(), layouts.Get(), MAX_FRAMES_IN_FLIGHT);
    //
    ////glm::vec3 color0 = glm::vec3(1.f, 0.f, 0.f);
    ////glm::vec3 color1 = glm::vec3(0.f, 1.f, 0.f);
    //phongShader.SetUBO0(glm::mat4(1.f), camera->GetViewMatrix(), camera->GetProjMatrix());
    //phongShader.SetUBO1(camera->GetPosition());
    //
    //cbvs0.resize(MAX_FRAMES_IN_FLIGHT);
    //cbvs1.resize(MAX_FRAMES_IN_FLIGHT);
    //BufferCreateInfo _infoUBO0;
    //_infoUBO0.size = phongShader.GetBufferSizeBinding(0);
    //BufferCreateInfo _infoUBO1;
    //_infoUBO1.size = phongShader.GetBufferSizeBinding(1);
    //
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    cbvs0[i].Create(device, _infoUBO0);
    //    cbvs1[i].Create(device, _infoUBO1);
    //}
    //
    //createTexture();
    //createSampler();
    //
    ////fs::path resourcePath = "F:/MVulkanEngine/resources/textures";
    ////fs::path imagePath = resourcePath / "test.jpg";
    ////image.Load(imagePath);
    ////
    ////ImageCreateInfo imageCreateInfo;
    ////imageCreateInfo.width = image.Width();
    ////imageCreateInfo.height = image.Height();
    ////imageCreateInfo.format = image.Format();
    ////imageCreateInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ////imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ////imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ////
    ////testTexture.CreateAndLoadData(&transferList, device, imageCreateInfo, &image);
    ////transferList.End();
    //
    //MVulkanDescriptorSetWrite write;
    //
    //std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(MAX_FRAMES_IN_FLIGHT);
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    bufferInfos[i].resize(2);
    //
    //    bufferInfos[i][0].buffer = cbvs0[i].GetBuffer();
    //    bufferInfos[i][0].offset = 0;
    //    bufferInfos[i][0].range = phongShader.GetBufferSizeBinding(0);
    //
    //    bufferInfos[i][1].buffer = cbvs1[i].GetBuffer();
    //    bufferInfos[i][1].offset = 0;
    //    bufferInfos[i][1].range = phongShader.GetBufferSizeBinding(1);
    //}
    //
    //std::vector < std::vector<VkDescriptorImageInfo>> imageInfos(MAX_FRAMES_IN_FLIGHT);
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    imageInfos[i].resize(1);
    //
    //    imageInfos[i][0].sampler = sampler.GetSampler();
    //    imageInfos[i][0].imageView = testTexture.GetImageView();
    //    imageInfos[i][0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //}
    //
    //write.Update(device.GetDevice(), descriptorSet.Get(), bufferInfos, imageInfos, MAX_FRAMES_IN_FLIGHT);
    //
    ////gbufferPipeline.Create(device, vert.GetShaderModule(), frag.GetShaderModule(), gbufferRenderPass.Get(), info, layouts.Get());
    //finalPipeline.Create(device, vert.GetShaderModule(), frag.GetShaderModule(), finalRenderPass.Get(), info, layouts.Get(), 1);
    //
    //vert.Clean(device.GetDevice());
    //frag.Clean(device.GetDevice());
}

//void MVulkanEngine::createDepthBuffer()
//{
//    VkExtent2D extent = swapChain.GetSwapChainExtent(); 
//    VkFormat format = device.FindDepthFormat();
//
//    depthBuffer.Create(device, extent, format);
//}

void MVulkanEngine::createGbufferFrameBuffers()
{
    std::vector<VkImageView> imageViews = swapChain.GetSwapChainImageViews();

    transferList.Reset();
    transferList.Begin();

    MVulkanImageMemoryBarrier barrier{};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    gbufferFrameBuffers.resize(imageViews.size());
    for (auto i = 0; i < imageViews.size(); i++) {
        std::vector<VkFormat> formats(2);
        formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT; //normal
        formats[1] = VK_FORMAT_R32G32B32A32_SFLOAT; //uv

        FrameBufferCreateInfo info{};

        info.useAttachmentResolve = false;
        info.renderPass = gbufferRenderPass.Get();
        info.extent = swapChain.GetSwapChainExtent();
        info.imageAttachmentFormats = formats.data();
        info.numAttachments = static_cast<uint32_t>(formats.size());
        info.useAttachmentResolve = false;
        //info.swapChainImageView = imageViews[i];

        gbufferFrameBuffers[i].Create(device, info);

        barrier.image = gbufferFrameBuffers[i].GetImage(0);

        transferList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    }

    transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();
}

void MVulkanEngine::createFrameBuffers()
{
    std::vector<VkImageView> imageViews = swapChain.GetSwapChainImageViews();

    transferList.Reset();
    transferList.Begin();

    finalFrameBuffers.resize(imageViews.size());
    for (auto i = 0; i < imageViews.size(); i++) {
        std::vector<VkFormat> formats(1);
        formats[0] = swapChain.GetSwapChainImageFormat();

        FrameBufferCreateInfo info{};

        info.finalFrameBuffer = true;
        info.useAttachmentResolve = true;
        info.renderPass = finalRenderPass.Get();
        info.extent = swapChain.GetSwapChainExtent();
        info.imageAttachmentFormats = formats.data();
        info.numAttachments = static_cast<uint32_t>(formats.size());
        info.colorAttachmentResolvedViews = &imageViews[i];
        info.useAttachmentResolve = false;

        finalFrameBuffers[i].Create(device, info);
    }
    transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();
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

    generalGraphicList.Create(device.GetDevice(), info);

    info.commandPool = commandAllocator.Get(QueueType::TRANSFER_QUEUE);
    transferList.Create(device.GetDevice(), info);

    info.commandPool = commandAllocator.Get(QueueType::PRESENT_QUEUE);
    presentList.Create(device.GetDevice(), info);
}

void MVulkanEngine::createCamera()
{
    glm::vec3 position(-1.f, 0.f, 4.f);
    glm::vec3 center(0.f);
    glm::vec3 direction = glm::normalize(center - position);

    float fov = 60;
    float aspectRatio = (float)windowWidth / (float)windowHeight;
    float zNear = 0.01f;
    float zFar = 1000.f;

    camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
}

void MVulkanEngine::createBufferAndLoadData()
{
    loadModel();

    BufferCreateInfo vertexInfo;
    vertexInfo.size = model->VertexSize();

    BufferCreateInfo indexInfo;
    indexInfo.size = model->IndexSize();
    
    transferList.Reset();
    transferList.Begin();
    vertexBuffer.CreateAndLoadData(&transferList, device, vertexInfo, (void*)(model->GetVerticesData()));
    indexBuffer.CreateAndLoadData(&transferList, device, indexInfo, (void*)model->GetIndicesData());

    vertexInfo.size = sizeof(sqad_vertices);
    indexInfo.size = sizeof(sqad_indices);
    sqadVertexBuffer.CreateAndLoadData(&transferList, device, vertexInfo, (void*)(sqad_vertices));
    sqadIndexBuffer.CreateAndLoadData(&transferList, device, indexInfo, (void*)(sqad_indices));

    transferList.End();
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();
    
    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();
}

void MVulkanEngine::createTexture()
{
    fs::path resourcePath = fs::current_path().parent_path().parent_path().append("resources").append("textures");
    fs::path imagePath = resourcePath / "texture.jpg";
    image.Load(imagePath);

    ImageCreateInfo imageinfo{};
    imageinfo.width = image.Width();
    imageinfo.height = image.Height();
    imageinfo.format = image.Format();
    imageinfo.mipLevels = image.MipLevels();
    imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageinfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    ImageViewCreateInfo viewInfo{};
    viewInfo.format = image.Format();
    viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.levelCount = image.MipLevels();

    generalGraphicList.Reset();
    generalGraphicList.Begin();
    testTexture.CreateAndLoadData(&generalGraphicList, device, imageinfo, viewInfo, &image);
    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();
    ///Submit
    //testTexture.Create(device, info);
}

void MVulkanEngine::createSampler()
{
    sampler.Create(device);
}

void MVulkanEngine::loadModel()
{
    model = std::make_shared<Model>();

    fs::path resourcePath = fs::current_path().parent_path().parent_path().append("resources").append("models");
    fs::path modelPath = resourcePath / "suzanne.obj";

    model->Load(modelPath.string());
}

void MVulkanEngine::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    gbufferRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    finalRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    gbufferTransferFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    transferFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        imageAvailableSemaphores[i].Create(device.GetDevice());
        renderFinishedSemaphores[i].Create(device.GetDevice());
        gbufferRenderFinishedSemaphores[i].Create(device.GetDevice());
        finalRenderFinishedSemaphores[i].Create(device.GetDevice());
        gbufferTransferFinishedSemaphores[i].Create(device.GetDevice());
        transferFinishedSemaphores[i].Create(device.GetDevice());
        inFlightFences[i].Create(device.GetDevice());
    }
}

void MVulkanEngine::recordFinalCommandBuffer(uint32_t imageIndex)
{
    graphicsLists[currentFrame].Begin();

    VkExtent2D swapChainExtent = swapChain.GetSwapChainExtent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = finalRenderPass.Get();
    renderPassInfo.framebuffer = finalFrameBuffers[imageIndex].Get();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    graphicsLists[currentFrame].BeginRenderPass(&renderPassInfo);

    graphicsLists[currentFrame].BindPipeline(finalPipeline.Get());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = (float)swapChainExtent.height;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = -(float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    graphicsLists[currentFrame].SetViewport(0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    graphicsLists[currentFrame].SetScissor(0, 1, &scissor);

    VkBuffer vertexBuffers[] = { sqadVertexBuffer.GetBuffer() };
    //VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };

    graphicsLists[currentFrame].BindVertexBuffers(0, 1, vertexBuffers, offsets);
    graphicsLists[currentFrame].BindIndexBuffers(0, 1, sqadIndexBuffer.GetBuffer(), offsets);

    auto descriptorSets = finalDescriptorSet.Get();
    graphicsLists[currentFrame].BindDescriptorSet(finalPipeline.GetLayout(), 0, 1, &descriptorSets[currentFrame]);

    graphicsLists[currentFrame].DrawIndexed(6, 1, 0, 0, 0);
    graphicsLists[currentFrame].EndRenderPass();
    graphicsLists[currentFrame].End();
}

void MVulkanEngine::renderPass(uint32_t currentFrame, uint32_t imageIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) {
    //vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    graphicsLists[currentFrame].Reset();

    recordGbufferCommandBuffer(imageIndex);

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

    //VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue.Get(), 1, &submitInfo, inFlightFences[currentFrame]));
    //VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue.Get(), 1, &submitInfo, nullptr));
    graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
    graphicsQueue.WaitForQueueComplete();
    //vkQueueWaitIdle(graphicsQueue.Get());
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

    //graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
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
    //graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
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

void MVulkanEngine::present(VkSwapchainKHR* swapChains, VkSemaphore* waitSemaphore, const uint32_t* imageIndex)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    //presentInfo.pWaitSemaphores = signalSemaphores2;
    presentInfo.pWaitSemaphores = waitSemaphore;

    //VkSwapchainKHR swapChains[] = { swapChain.Get() };
    //presentInfo.swapchainCount = 1;
    //presentInfo.pSwapchains = swapChains;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = imageIndex;

    VkResult result = presentQueue.Present(&presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window->GetWindowResized()) {
        window->SetWindowResized(false);
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void MVulkanEngine::recordGbufferCommandBuffer(uint32_t imageIndex)
{
    graphicsLists[currentFrame].Begin();

    VkExtent2D swapChainExtent = swapChain.GetSwapChainExtent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = gbufferRenderPass.Get();
    renderPassInfo.framebuffer = gbufferFrameBuffers[imageIndex].Get();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[2].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    graphicsLists[currentFrame].BeginRenderPass(&renderPassInfo);

    graphicsLists[currentFrame].BindPipeline(gbufferPipeline.Get());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = (float)swapChainExtent.height;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = -(float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    graphicsLists[currentFrame].SetViewport(0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    graphicsLists[currentFrame].SetScissor(0, 1, &scissor);

    VkBuffer vertexBuffers[] = { vertexBuffer.GetBuffer()};
    //VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };

    graphicsLists[currentFrame].BindVertexBuffers(0, 1, vertexBuffers, offsets);
    graphicsLists[currentFrame].BindIndexBuffers(0, 1, indexBuffer.GetBuffer(), offsets);

    auto descriptorSets = gbufferDescriptorSet.Get();
    graphicsLists[currentFrame].BindDescriptorSet(gbufferPipeline.GetLayout(), 0, 1, &descriptorSets[currentFrame]);

    GbufferShader::UniformBufferObject ubo{};
    ubo.Model = glm::mat4(1.f);
    ubo.View = camera->GetViewMatrix();
    ubo.Projection = camera->GetProjMatrix();
    gbufferShader->SetUBO(0, &ubo);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        gbufferCbvs0[i].UpdateData(device, gbufferShader->GetData(0));
        //cbvs1[i].UpdateData(device, gbufferShader.GetData(1));
    }
    //graphicsLists[currentFrame].DrawIndexed(sizeof(indices) / sizeof(uint32_t), 1, 0, 0, 0);
    graphicsLists[currentFrame].DrawIndexed(static_cast<uint32_t>(model->GetIndices().size()), 1, 0, 0, 0);

    //testShader.SetUBO0(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -1.f)), camera->GetViewMatrix(), camera->GetProjMatrix());
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    cbvs0[i].UpdateData(device, testShader.GetData(0));
    //    cbvs1[i].UpdateData(device, testShader.GetData(1));
    //}
    //graphicsLists[currentFrame].DrawIndexed(6, 1, 0, 0, 0);

    graphicsLists[currentFrame].EndRenderPass();

    graphicsLists[currentFrame].End();
}

std::vector<VkDescriptorBufferInfo> MVulkanEngine::generateDescriptorBufferInfos(std::vector<VkBuffer> buffers, std::vector<ShaderResourceInfo> resourceInfos)
{
    std::vector<VkDescriptorBufferInfo> bufferInfos;

    for (auto i = 0; i < resourceInfos.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffers[i];
        bufferInfo.offset = resourceInfos[i].offset;
        bufferInfo.range = resourceInfos[i].size;

        bufferInfos.push_back(bufferInfo);
    }

    return bufferInfos;
}


