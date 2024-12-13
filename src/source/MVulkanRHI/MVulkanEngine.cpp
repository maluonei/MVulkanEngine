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
    0, 2, 1,
    2, 3, 1
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

    preComputeIrradianceCubemap();
    preFilterEnvmaps();
    preComputeLUT();

    createLight();
    createCamera();
    //preComputeIrradianceCubemap();
}

void MVulkanEngine::Clean()
{
    window->Clean();

    commandAllocator.Clean(device.GetDevice());

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
    inFlightFences[currentFrame].WaitForSignal(device.GetDevice());

    uint32_t imageIndex;
    VkResult result = swapChain.AcquireNextImage(device.GetDevice(), imageAvailableSemaphores[0].GetSemaphore(), VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    inFlightFences[0].Reset(device.GetDevice());

    //prepare gbufferPass ubo
    {
        GbufferShader::UniformBufferObject0 ubo0{};
        ubo0.Model = glm::mat4(1.f);
        ubo0.View = camera->GetViewMatrix();
        ubo0.Projection = camera->GetProjMatrix();
        gbufferPass->GetShader()->SetUBO(0, &ubo0);

        GbufferShader::UniformBufferObject1 ubo1[256];

        std::vector<GbufferShader::UniformBufferObject2> ubo2(4);
        ubo2[0].testValue = glm::vec4(1.f, 0.f, 0.f, 0.f);
        ubo2[1].testValue = glm::vec4(0.f, 1.f, 0.f, 0.f);
        ubo2[2].testValue = glm::vec4(0.f, 0.f, 1.f, 0.f);
        ubo2[3].testValue = glm::vec4(0.f, 0.f, 0.f, 1.f);
        gbufferPass->GetShader()->SetUBO(2, ubo2.data());

        auto meshNames = scene->GetMeshNames();
        auto drawIndexedIndirectCommands = scene->GetIndirectDrawCommands();

        for (auto i = 0; i < meshNames.size(); i++) {
            auto name = meshNames[i];
            auto mesh = scene->GetMesh(name);
            auto mat = scene->GetMaterial(mesh->matId);
            auto indirectCommand = drawIndexedIndirectCommands[i];
            if (mat->diffuseTexture != "") {
                auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
                ubo1[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
            }
            else {
                ubo1[indirectCommand.firstInstance].diffuseTextureIdx = -1;
            }

            if (mat->metallicAndRoughnessTexture != "") {
                auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
                ubo1[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = metallicAndRoughnessTexId;
            }
            else {
                ubo1[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = -1;
            }

            if (i == 25) {
                ubo1[indirectCommand.firstInstance].matId = 1;
            }
            else {
                ubo1[indirectCommand.firstInstance].matId = 0;
            }
        }
        gbufferPass->GetShader()->SetUBO(1, &ubo1);
    }

    //prepare shadowPass ubo
    {
        ShadowShader::ShadowUniformBuffer ubo0{};
        ubo0.shadowMVP = directionalLightCamera->GetOrthoMatrix() * directionalLightCamera->GetViewMatrix();
        shadowPass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare lightingPass ubo
    {
        LightingPbrShader::UniformBuffer0 ubo0{};
        ubo0.ResolusionWidth = shadowPass->GetFrameBuffer(0).GetExtent2D().width;
        ubo0.ResolusionHeight = shadowPass->GetFrameBuffer(0).GetExtent2D().height;

        LightingPbrShader::DirectionalLightBuffer ubo1{};
        ubo1.lightNum = 1;
        ubo1.lights[0].direction = directionalLightCamera->GetDirection();
        ubo1.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(directionalLight)->GetIntensity();
        ubo1.lights[0].color = std::static_pointer_cast<DirectionalLight>(directionalLight)->GetColor();
        ubo1.lights[0].shadowMapIndex = 0;
        ubo1.lights[0].shadowViewProj = directionalLightCamera->GetOrthoMatrix() * directionalLightCamera->GetViewMatrix();
        ubo1.lights[0].cameraZnear = directionalLightCamera->GetZnear();
        ubo1.lights[0].cameraZfar = directionalLightCamera->GetZfar();
        ubo1.cameraPos = camera->GetPosition();

        lightingPass->GetShader()->SetUBO(0, &ubo0);
        lightingPass->GetShader()->SetUBO(1, &ubo1);
    }

    {
        SkyboxShader::UniformBuffer0 ubo0{};
        ubo0.View = camera->GetViewMatrix();
        ubo0.Projection = camera->GetProjMatrix();

        skyboxPass->GetShader()->SetUBO(0, &ubo0);
    }

    graphicsLists[currentFrame].Reset();
    graphicsLists[currentFrame].Begin();

    recordCommandBuffer(0, shadowPass, graphicsLists[currentFrame], scene->GetIndirectVertexBuffer(), scene->GetIndirectIndexBuffer(), scene->GetIndirectBuffer(), scene->GetIndirectDrawCommands().size());
    recordCommandBuffer(0, gbufferPass,graphicsLists[currentFrame], scene->GetIndirectVertexBuffer(), scene->GetIndirectIndexBuffer(), scene->GetIndirectBuffer(), scene->GetIndirectDrawCommands().size());
    //recordCommandBuffer(0, shadowPass, graphicsLists[currentFrame], sphere->GetIndirectVertexBuffer(), sphere->GetIndirectIndexBuffer(), sphere->GetIndirectBuffer(), sphere->GetIndirectDrawCommands().size());
    //recordCommandBuffer(0, gbufferPass, graphicsLists[currentFrame], sphere->GetIndirectVertexBuffer(), sphere->GetIndirectIndexBuffer(), sphere->GetIndirectBuffer(), sphere->GetIndirectDrawCommands().size());
    recordCommandBuffer(imageIndex, lightingPass, graphicsLists[currentFrame], squadVertexBuffer, squadIndexBuffer, lightPassIndirectBuffer, 1);
    recordCommandBuffer(imageIndex, skyboxPass, graphicsLists[currentFrame], cube->GetIndirectVertexBuffer(), cube->GetIndirectIndexBuffer(), cube->GetIndirectBuffer(), cube->GetIndirectDrawCommands().size());

    graphicsLists[currentFrame].End();
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores3[] = { imageAvailableSemaphores[0].GetSemaphore() };
    VkPipelineStageFlags waitStages3[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores3;
    submitInfo.pWaitDstStageMask = waitStages3;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores3[] = { finalRenderFinishedSemaphores[0].GetSemaphore() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores3;

    graphicsQueue.SubmitCommands(1, &submitInfo, inFlightFences[0].GetFence());
    graphicsQueue.WaitForQueueComplete();

    VkSemaphore transferSignalSemaphores3[] = { finalRenderFinishedSemaphores[0].GetSemaphore() };

    vkDeviceWaitIdle(device.GetDevice());
    present(swapChain.GetPtr(), transferSignalSemaphores3, &imageIndex);

    currentFrame = (currentFrame + 1) % Singleton<GlobalConfig>::instance().GetMaxFramesInFlight();
}

void MVulkanEngine::createGlobalSamplers()
{
    createSampler();

    globalSamplers.push_back(linearSampler);
    globalSamplers.push_back(nearestSampler);
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

MVulkanSampler MVulkanEngine::GetGlobalSampler() const
{
    return globalSamplers[0];
}

void MVulkanEngine::initVulkan()
{
    createInstance();
    createSurface();
    createDevice();

    createGlobalSamplers();

    createCommandQueue();
    createCommandAllocator();
    createCommandList();
    createSyncObjects();

    createTempTexture();
    createSkyboxTexture();
    createIrradianceCubemapTexture();

    createSwapChain();
    
    createDescriptorSetAllocator();

    createBufferAndLoadData();

    createRenderPass();
}

void MVulkanEngine::renderLoop()
{
    window->WindowPollEvents();
    Singleton<InputManager>::instance().DealInputs();
    drawFrame();
    //m_frameId++;
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

void MVulkanEngine::RecreateSwapChain()
{

    if (swapChain.Recreate(device, window->GetWindow(), surface)) {
        gbufferPass->RecreateFrameBuffers(swapChain, transferQueue, transferList, swapChain.GetSwapChainExtent());
        shadowPass->RecreateFrameBuffers(swapChain, transferQueue, transferList, swapChain.GetSwapChainExtent());
        
        lightingPass->GetRenderPassCreateInfo().depthView = gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        lightingPass->RecreateFrameBuffers(swapChain, transferQueue, transferList, swapChain.GetSwapChainExtent());

        skyboxPass->GetRenderPassCreateInfo().depthView = gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        skyboxPass->RecreateFrameBuffers(swapChain, transferQueue, transferList, swapChain.GetSwapChainExtent());

        //gbufferPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        //shadowPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        //lightingPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        //skyboxPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        std::vector<std::vector<VkImageView>> gbufferViews(9);
        for (auto i = 0; i < 5; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        gbufferViews[5] = std::vector<VkImageView>(2);
        gbufferViews[5][0] = shadowPass->GetFrameBuffer(0).GetDepthImageView();
        gbufferViews[5][1] = shadowPass->GetFrameBuffer(0).GetDepthImageView();
        gbufferViews[6] = std::vector<VkImageView>(1);
        gbufferViews[6][0] = irradianceTexture.GetImageView();
        gbufferViews[7] = std::vector<VkImageView>(1);
        gbufferViews[7][0] = preFilteredEnvTexture.GetImageView();
        gbufferViews[8] = std::vector<VkImageView>(1);
        gbufferViews[8][0] = brdfLUTPass->GetFrameBuffer(0).GetImageView(0);

        std::vector<VkSampler> samplers(9, linearSampler.GetSampler());
        samplers[4] = nearestSampler.GetSampler();

        lightingPass->UpdateDescriptorSetWrite(gbufferViews, samplers);
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

void MVulkanEngine::createRenderPass()
{

    {
        RenderPassCreateInfo irradianceConvolusionPassInfo{};
        std::vector<VkFormat> irradianceConvolutionPassFormats(0);
        irradianceConvolutionPassFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);

        irradianceConvolusionPassInfo.extent = VkExtent2D(512, 512);
        irradianceConvolusionPassInfo.depthFormat = device.FindDepthFormat();
        irradianceConvolusionPassInfo.frambufferCount = 1;
        irradianceConvolusionPassInfo.useSwapchainImages = false;
        irradianceConvolusionPassInfo.imageAttachmentFormats = irradianceConvolutionPassFormats;
        irradianceConvolusionPassInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        irradianceConvolusionPassInfo.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        irradianceConvolusionPassInfo.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        irradianceConvolusionPassInfo.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        irradianceConvolusionPassInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        irradianceConvolusionPassInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        irradianceConvolusionPassInfo.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        irradianceConvolusionPassInfo.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        irradianceConvolusionPassInfo.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
        irradianceConvolusionPassInfo.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        irradianceConvolusionPassInfo.depthView = nullptr;

        irradianceConvolutionPass = std::make_shared<RenderPass>(device, irradianceConvolusionPassInfo);

        std::shared_ptr<ShaderModule> irradianceConvolusionShader = std::make_shared<IrradianceConvolutionShader>();
        std::vector<std::vector<VkImageView>> irradianceConvolusionPassViews(1);
        irradianceConvolusionPassViews[0] = std::vector<VkImageView>(1);
        irradianceConvolusionPassViews[0][0] = skyboxTexture.GetImageView();

        std::vector<VkSampler> samplers(1, linearSampler.GetSampler());

        irradianceConvolutionPass->Create(irradianceConvolusionShader, swapChain, graphicsQueue, generalGraphicList, allocator, irradianceConvolusionPassViews, samplers);
    }

    {
        RenderPassCreateInfo prefilterEnvmapPassInfo{};
        std::vector<VkFormat> prefilterEnvmapPassFormats(0);
        prefilterEnvmapPassFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);

        prefilterEnvmapPassInfo.extent = VkExtent2D(128, 128);
        prefilterEnvmapPassInfo.depthFormat = device.FindDepthFormat();
        prefilterEnvmapPassInfo.frambufferCount = 1;
        prefilterEnvmapPassInfo.useSwapchainImages = false;
        prefilterEnvmapPassInfo.imageAttachmentFormats = prefilterEnvmapPassFormats;
        prefilterEnvmapPassInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        prefilterEnvmapPassInfo.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        prefilterEnvmapPassInfo.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        prefilterEnvmapPassInfo.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        prefilterEnvmapPassInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        prefilterEnvmapPassInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        prefilterEnvmapPassInfo.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        prefilterEnvmapPassInfo.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        prefilterEnvmapPassInfo.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
        prefilterEnvmapPassInfo.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        prefilterEnvmapPassInfo.depthView = nullptr;

        prefilterEnvmapPass = std::make_shared<RenderPass>(device, prefilterEnvmapPassInfo);

        std::shared_ptr<ShaderModule> prefilterEnvmapShader = std::make_shared<PreFilterEnvmapShader>();
        std::vector<std::vector<VkImageView>> prefilterEnvmapPassViews(1);
        prefilterEnvmapPassViews[0] = std::vector<VkImageView>(1);
        prefilterEnvmapPassViews[0][0] = skyboxTexture.GetImageView();

        std::vector<VkSampler> samplers(1, linearSampler.GetSampler());

        prefilterEnvmapPass->Create(prefilterEnvmapShader, swapChain, graphicsQueue, generalGraphicList, allocator, prefilterEnvmapPassViews, samplers);
    }

    {
        RenderPassCreateInfo brdfLUTPassInfo{};
        std::vector<VkFormat> brdfLUTPassFormats(0);
        brdfLUTPassFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);

        brdfLUTPassInfo.extent = VkExtent2D(512, 512);
        brdfLUTPassInfo.depthFormat = device.FindDepthFormat();
        brdfLUTPassInfo.frambufferCount = 1;
        brdfLUTPassInfo.useSwapchainImages = false;
        brdfLUTPassInfo.imageAttachmentFormats = brdfLUTPassFormats;
        brdfLUTPassInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        brdfLUTPassInfo.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        brdfLUTPassInfo.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        brdfLUTPassInfo.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        brdfLUTPassInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        brdfLUTPassInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        brdfLUTPassInfo.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        brdfLUTPassInfo.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        brdfLUTPassInfo.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
        brdfLUTPassInfo.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        brdfLUTPassInfo.depthView = nullptr;

        brdfLUTPass = std::make_shared<RenderPass>(device, brdfLUTPassInfo);

        std::shared_ptr<ShaderModule> brdfLUTShader = std::make_shared<IBLBrdfShader>();
        std::vector<std::vector<VkImageView>> brdfLUTPassViews(0);

        std::vector<VkSampler> samplers(0);

        brdfLUTPass->Create(brdfLUTShader, swapChain, graphicsQueue, generalGraphicList, allocator, brdfLUTPassViews, samplers);
    }

    {
        std::vector<VkFormat> gbufferFormats;
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);

        RenderPassCreateInfo info{};
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.extent = swapChain.GetSwapChainExtent();
        info.useAttachmentResolve = false;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = gbufferFormats;
        info.colorAttachmentResolvedViews = nullptr;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;
        info.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;

        gbufferPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> gbufferShader = std::make_shared<GbufferShader>();
        std::vector<std::vector<VkImageView>> bufferTextureViews(1);
        auto wholeTextures = Singleton<TextureManager>::instance().GenerateTextureVector();
        auto wholeTextureSize = wholeTextures.size();
        for (auto i = 0; i < bufferTextureViews.size(); i++) {
            bufferTextureViews[i].resize(wholeTextureSize);
            for (auto j = 0; j < wholeTextureSize; j++) {
                bufferTextureViews[i][j] = wholeTextures[j]->GetImageView();
            }
        }

        std::vector<VkSampler> samplers(wholeTextures.size(), linearSampler.GetSampler());

        gbufferPass->Create(gbufferShader, swapChain, graphicsQueue, generalGraphicList, allocator, bufferTextureViews, samplers);
    }

    {
        std::vector<VkFormat> shadowFormats(0);
        shadowFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        RenderPassCreateInfo info{};
        info.depthFormat = VK_FORMAT_D16_UNORM;
        info.frambufferCount = 1;
        info.extent = VkExtent2D(4096, 4096);
        info.useAttachmentResolve = false;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = shadowFormats;
        info.colorAttachmentResolvedViews = nullptr;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        shadowPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> shadowShader = std::make_shared<ShadowShader>();
        std::vector<std::vector<VkImageView>> shadowShaderTextures(1);
        shadowShaderTextures[0].resize(0);

        std::vector<VkSampler> samplers(0);

        shadowPass->Create(shadowShader, swapChain, graphicsQueue, generalGraphicList, allocator, shadowShaderTextures, samplers);
    }

    {
        std::vector<VkFormat> lightingPassFormats;
        lightingPassFormats.push_back(swapChain.GetSwapChainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = swapChain.GetSwapChainExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = swapChain.GetImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = lightingPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.reuseDepthView = true;
        info.depthView = gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        lightingPass = std::make_shared<RenderPass>(device, info);

        //std::shared_ptr<ShaderModule> lightingShader = std::make_shared<SquadPhongShader>();
        std::shared_ptr<ShaderModule> lightingShader = std::make_shared<LightingIBLShader>();
        std::vector<std::vector<VkImageView>> gbufferViews(9);
        for (auto i = 0; i < 5; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        gbufferViews[5] = std::vector<VkImageView>(2);
        gbufferViews[5][0] = shadowPass->GetFrameBuffer(0).GetDepthImageView();
        gbufferViews[5][1] = shadowPass->GetFrameBuffer(0).GetDepthImageView();
        gbufferViews[6] = std::vector<VkImageView>(1);
        gbufferViews[6][0] = irradianceTexture.GetImageView();
        gbufferViews[7] = std::vector<VkImageView>(1);
        gbufferViews[7][0] = preFilteredEnvTexture.GetImageView();
        gbufferViews[8] = std::vector<VkImageView>(1);
        gbufferViews[8][0] = brdfLUTPass->GetFrameBuffer(0).GetImageView(0);

        std::vector<VkSampler> samplers(9, linearSampler.GetSampler());
        samplers[4] = nearestSampler.GetSampler();

        lightingPass->Create(lightingShader, swapChain, graphicsQueue, generalGraphicList, allocator, gbufferViews, samplers);
        //lightingPass->GetFrameBuffer().GetDepthImage
    }

    {
        std::vector<VkFormat> skyboxPassFormats(0);
        skyboxPassFormats.push_back(swapChain.GetSwapChainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = swapChain.GetSwapChainExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = swapChain.GetImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = skyboxPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;
        info.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
        info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        info.depthView = gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        skyboxPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> skyboxShader = std::make_shared<SkyboxShader>();
        std::vector<std::vector<VkImageView>> skyboxPassViews(1);
        skyboxPassViews[0] = std::vector<VkImageView>(1);
        skyboxPassViews[0][0] = skyboxTexture.GetImageView();

        std::vector<VkSampler> samplers(1, linearSampler.GetSampler());

        skyboxPass->Create(skyboxShader, swapChain, graphicsQueue, generalGraphicList, allocator, skyboxPassViews, samplers);
    }
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

void MVulkanEngine::createCamera()
{
    glm::vec3 position(-1.f, 0.f, 4.f);
    glm::vec3 center(0.f);
    glm::vec3 direction = glm::normalize(center - position);

    float fov = 60.f;
    float aspectRatio = (float)windowWidth / (float)windowHeight;
    float zNear = 0.01f;
    float zFar = 1000.f;

    camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    //spdlog::info("create camera");
}

void MVulkanEngine::createBufferAndLoadData()
{
    loadScene();

    transferList.Reset();
    transferList.Begin();

    BufferCreateInfo vertexInfo;
    BufferCreateInfo indexInfo;

    vertexInfo.size = sizeof(sqad_vertices);
    indexInfo.size = sizeof(sqad_indices);

    squadVertexBuffer = std::make_shared<Buffer>(BufferType::VERTEX_BUFFER);
    squadIndexBuffer = std::make_shared<Buffer>(BufferType::INDEX_BUFFER);
    squadVertexBuffer->CreateAndLoadData(&transferList, device, vertexInfo, (void*)(sqad_vertices));
    squadIndexBuffer->CreateAndLoadData(&transferList, device, indexInfo, (void*)(sqad_indices));

    BufferCreateInfo lightPassIndirectBufferInfo;
    lightPassIndirectBufferInfo.size = sizeof(VkDrawIndexedIndirectCommand);

    VkDrawIndexedIndirectCommand lightPassIndirectBufferCommand;
    lightPassIndirectBufferCommand.indexCount = 6;
    lightPassIndirectBufferCommand.instanceCount = 1;
    lightPassIndirectBufferCommand.firstIndex = 0;
    lightPassIndirectBufferCommand.vertexOffset = 0;
    lightPassIndirectBufferCommand.firstInstance = 0;

    lightPassIndirectBuffer = std::make_shared<Buffer>(BufferType::INDIRECT_BUFFER);
    lightPassIndirectBuffer->CreateAndLoadData(&transferList, device, lightPassIndirectBufferInfo, &lightPassIndirectBufferCommand);

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
    std::vector<MImage<unsigned char>*> images(1);
    images[0] = &image;
    testTexture.CreateAndLoadData(&generalGraphicList, device, imageinfo, viewInfo, images);
    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::createSkyboxTexture()
{
    fs::path resourcePath = fs::current_path().parent_path().parent_path().append("resources").append("textures").append("noon_grass");
    
    std::vector<MImage<unsigned char>*> images(6);

    std::vector<std::shared_ptr<MImage<unsigned char>>> imagesPtrs(6);

    //std::shared_ptr<MImage<unsigned char>> m_image = nullptr;
    for (auto i = 0; i < 6; i++) {
        imagesPtrs[i] = std::make_shared<MImage<unsigned char>>();

        fs::path imagePath = resourcePath / (std::to_string(i) + ".png");
        //spdlog::info(imagePath.string());
        imagesPtrs[i]->Load(imagePath);

        images[i] = imagesPtrs[i].get();
    }

    ImageCreateInfo imageinfo{};
    imageinfo.width = imagesPtrs[0]->Width();
    imageinfo.height = imagesPtrs[0]->Height();
    imageinfo.format = imagesPtrs[0]->Format();
    imageinfo.mipLevels = imagesPtrs[0]->MipLevels();
    imageinfo.arrayLength = 6;
    imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageinfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageinfo.flag = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    ImageViewCreateInfo viewInfo{};
    viewInfo.format = imagesPtrs[0]->Format();
    viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    //viewInfo.levelCount = m_image->MipLevels();
    viewInfo.levelCount = 1;
    viewInfo.layerCount = 6;

    generalGraphicList.Reset();
    generalGraphicList.Begin();
    //std::vector<MImage<unsigned char>*> images(1);
    //images[0] = &image;
    skyboxTexture.CreateAndLoadData(&generalGraphicList, device, imageinfo, viewInfo, images);
    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::createIrradianceCubemapTexture()
{
    {
        ImageCreateInfo imageinfo{};
        imageinfo.width = 512;
        imageinfo.height = 512;
        imageinfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageinfo.mipLevels = 1;
        imageinfo.arrayLength = 6;
        imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageinfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageinfo.flag = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        ImageViewCreateInfo viewInfo{};
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        //viewInfo.levelCount = m_image->MipLevels();
        viewInfo.levelCount = 1;
        viewInfo.layerCount = 6;

        generalGraphicList.Reset();
        generalGraphicList.Begin();

        irradianceTexture.Create(&generalGraphicList, device, imageinfo, viewInfo);
        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        graphicsQueue.WaitForQueueComplete();
    }
    //return;

    //spdlog::info("aaaaaaaaaa");

    {
        ImageCreateInfo imageinfo{};
        imageinfo.width = 128;
        imageinfo.height = 128;
        imageinfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageinfo.mipLevels = 5;
        imageinfo.arrayLength = 6;
        imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageinfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageinfo.flag = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        ImageViewCreateInfo viewInfo{};
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.levelCount = 5;
        viewInfo.layerCount = 6;

        generalGraphicList.Reset();
        generalGraphicList.Begin();

        preFilteredEnvTexture.Create(&generalGraphicList, device, imageinfo, viewInfo);
        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        graphicsQueue.WaitForQueueComplete();
    }

    //spdlog::info("bbbbbbbbbbb");

    //{
    //    ImageCreateInfo imageinfo{};
    //    imageinfo.width = 512;
    //    imageinfo.height = 512;
    //    imageinfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    //    imageinfo.mipLevels = 1;
    //    imageinfo.arrayLength = 1;
    //    imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    //    imageinfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //    imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    //    //imageinfo.flag = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    //
    //    ImageViewCreateInfo viewInfo{};
    //    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    //    viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    //    viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    //    //viewInfo.levelCount = m_image->MipLevels();
    //    viewInfo.levelCount = 1;
    //    viewInfo.layerCount = 1;
    //
    //    generalGraphicList.Reset();
    //    generalGraphicList.Begin();
    //
    //    brdfLUTTexture.Create(&generalGraphicList, device, imageinfo, viewInfo);
    //    generalGraphicList.End();
    //
    //    VkSubmitInfo submitInfo{};
    //    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //    submitInfo.commandBufferCount = 1;
    //    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();
    //
    //    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    //    graphicsQueue.WaitForQueueComplete();
    //}

    //spdlog::info("ccccccccccc");
}

void MVulkanEngine::preComputeIrradianceCubemap()
{
    glm::vec3 directions[6] = {
        glm::vec3(1.f, 0.f, 0.f),
        glm::vec3(-1.f, 0.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, -1.f, 0.f),
        glm::vec3(0.f, 0.f, 1.f),
        glm::vec3(0.f, 0.f, -1.f)
    };

    glm::vec3 ups[6] = {
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, 0.f, 1.f),
        glm::vec3(0.f, 0.f, -1.f),
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f)
    };

    for (int i = 0; i < 6; i++) {
        glm::vec3 position(0.f, 0.f, 0.f);

        float fov = 90.f;
        float aspectRatio = 1.;
        float zNear = 0.01f;
        float zFar = 1000.f;

        std::shared_ptr<Camera> cam = std::make_shared<Camera>(position, directions[i], ups[i], fov, aspectRatio, zNear, zFar);

        {
            IrradianceConvolutionShader::UniformBuffer0 ubo0{};
            ubo0.View = cam->GetViewMatrix();
            ubo0.Projection = cam->GetProjMatrix();

            irradianceConvolutionPass->GetShader()->SetUBO(0, &ubo0);
        }

        generalGraphicList.Reset();
        generalGraphicList.Begin();

        recordCommandBuffer(0, irradianceConvolutionPass, generalGraphicList, cube->GetIndirectVertexBuffer(), cube->GetIndirectIndexBuffer(), cube->GetIndirectBuffer(), cube->GetIndirectDrawCommands().size());

        MVulkanImageCopyInfo copyInfo{};
        copyInfo.srcAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyInfo.dstAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyInfo.srcMipLevel = 0;
        copyInfo.dstMipLevel = 0;
        copyInfo.srcArrayLayer = 0;
        copyInfo.dstArrayLayer = i;
        copyInfo.layerCount = 1;

        generalGraphicList.CopyImage(irradianceConvolutionPass->GetFrameBuffer(0).GetImage(0), irradianceTexture.GetImage(), 512, 512, copyInfo);

        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
        graphicsQueue.WaitForQueueComplete();
    }
}

void MVulkanEngine::preFilterEnvmaps()
{
    glm::vec3 directions[6] = {
        glm::vec3(1.f, 0.f, 0.f),
        glm::vec3(-1.f, 0.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, -1.f, 0.f),
        glm::vec3(0.f, 0.f, 1.f),
        glm::vec3(0.f, 0.f, -1.f)
    };

    glm::vec3 ups[6] = {
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, 0.f, 1.f),
        glm::vec3(0.f, 0.f, -1.f),
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f)
    };

    for (auto mipLevel = 0; mipLevel < 5; mipLevel++) {
        //spdlog::info("mipLevel:{0}",  mipLevel);
        float roughness = mipLevel / 4.0f;
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mipLevel));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mipLevel));

        for (int i = 0; i < 6; i++) {
            //spdlog::info("layer:{0}", i);
            glm::vec3 position(0.f, 0.f, 0.f);

            float fov = 90.f;
            float aspectRatio = 1.;
            float zNear = 0.01f;
            float zFar = 1000.f;

            std::shared_ptr<Camera> cam = std::make_shared<Camera>(position, directions[i], ups[i], fov, aspectRatio, zNear, zFar);
            {
                PreFilterEnvmapShader::UniformBuffer0 ubo0{};
                ubo0.View = cam->GetViewMatrix();
                ubo0.Projection = cam->GetProjMatrix();

                PreFilterEnvmapShader::UniformBuffer1 ubo1{};
                ubo1.roughness = roughness;

                prefilterEnvmapPass->GetShader()->SetUBO(0, &ubo0);
                prefilterEnvmapPass->GetShader()->SetUBO(1, &ubo1);
            }

            generalGraphicList.Reset();
            generalGraphicList.Begin();

            recordCommandBuffer(0, prefilterEnvmapPass, generalGraphicList, cube->GetIndirectVertexBuffer(), cube->GetIndirectIndexBuffer(), cube->GetIndirectBuffer(), cube->GetIndirectDrawCommands().size());

            MVulkanImageCopyInfo copyInfo{};
            copyInfo.srcAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyInfo.dstAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyInfo.srcMipLevel = 0;
            copyInfo.dstMipLevel = mipLevel;
            copyInfo.srcArrayLayer = 0;
            copyInfo.dstArrayLayer = i;
            copyInfo.layerCount = 1;

            generalGraphicList.CopyImage(prefilterEnvmapPass->GetFrameBuffer(0).GetImage(0), preFilteredEnvTexture.GetImage(), mipWidth, mipHeight, copyInfo);

            generalGraphicList.End();

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

            graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
            graphicsQueue.WaitForQueueComplete();
        }
    }
}

void MVulkanEngine::preComputeLUT()
{
    generalGraphicList.Reset();
    generalGraphicList.Begin();

    recordCommandBuffer(0, brdfLUTPass, generalGraphicList, squadVertexBuffer, squadIndexBuffer, lightPassIndirectBuffer, 1);

    //MVulkanImageCopyInfo copyInfo{};
    //copyInfo.srcAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //copyInfo.dstAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //copyInfo.srcMipLevel = 0;
    //copyInfo.dstMipLevel = 0;
    //copyInfo.srcArrayLayer = 0;
    //copyInfo.dstArrayLayer = i;
    //copyInfo.layerCount = 1;
    //
    //generalGraphicList.CopyImage(brdfLUTPass->GetFrameBuffer(0).GetImage(0), brdfLUTTexture.GetImage(), 512, 512, copyInfo);

    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
    graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::createSampler()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        linearSampler.Create(device, info);
    }

    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_NEAREST;
        info.magFilter = VK_FILTER_NEAREST;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        nearestSampler.Create(device, info);
    }
}

void MVulkanEngine::loadScene()
{
    scene = std::make_shared<Scene>();

    fs::path resourcePath = fs::current_path().parent_path().parent_path().append("resources").append("models");
    //fs::path modelPath = resourcePath / "suzanne.obj";
    //fs::path modelPath = resourcePath / "CornellBox.glb";
    fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";

    Singleton<SceneLoader>::instance().Load(modelPath.string(), scene.get());

    BoundingBox bbx = scene->GetBoundingBox();

    //split Image
    {
        auto wholeTextures = Singleton<TextureManager>::instance().GenerateTextureVector();

        transferList.Reset();
        transferList.Begin();
        std::vector<MVulkanImageMemoryBarrier> barriers(wholeTextures.size());
        for (auto i = 0; i < wholeTextures.size(); i++) {
            MVulkanImageMemoryBarrier barrier{};
            barrier.image = wholeTextures[i]->GetImage();

            barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.levelCount = wholeTextures[i]->GetImageInfo().mipLevels;

            barriers[i] = barrier;
        }

        transferList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

        transferList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &transferList.GetBuffer();

        transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        transferQueue.WaitForQueueComplete();


        //auto meshNames = scene->GetMeshNames();
        for (auto item : wholeTextures) {
            auto texture = item;
            GenerateMipMap(*texture);
        }
    }

    cube = std::make_shared<Scene>();
    fs::path cubePath = resourcePath / "cube.obj";
    Singleton<SceneLoader>::instance().Load(cubePath.string(), cube.get());

    sphere = std::make_shared<Scene>();
    fs::path spherePath = resourcePath / "sphere.obj";
    Singleton<SceneLoader>::instance().Load(spherePath.string(), sphere.get());
    
    glm::mat4 translation = glm::translate(glm::mat4(1.f), glm::vec3(40.f, 5.f, 0.f));
    glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(2.f, 2.f, 2.f));
    scene->AddScene(sphere, translation * scale);

    cube->GenerateIndirectDataAndBuffers();
    scene->GenerateIndirectDataAndBuffers();
}

void MVulkanEngine::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 10.f;
    directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);

    {
        VkExtent2D extent = shadowPass->GetFrameBuffer(0).GetExtent2D();

        glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(directionalLight)->GetDirection();
        glm::vec3 position = -50.f * direction;

        float fov = 60.f;
        float aspect = (float)extent.width / (float)extent.height;
        float zNear = 0.001f;
        float zFar = 60.f;
        directionalLightCamera = std::make_shared<Camera>(position, direction, fov, aspect, zNear, zFar);
        directionalLightCamera->SetOrth(true);
    }
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

void MVulkanEngine::present(VkSwapchainKHR* swapChains, VkSemaphore* waitSemaphore, const uint32_t* imageIndex)
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
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void MVulkanEngine::createTempTexture()
{
    std::shared_ptr<MVulkanTexture> texture = std::make_shared<MVulkanTexture>();

    MImage<unsigned char> tempImage = MImage<unsigned char>::GetDefault();
    std::vector<MImage<unsigned char>*> images(1);
    images[0] = &tempImage;
    CreateImage(texture, images);

    Singleton<TextureManager>::instance().Put("placeholder", texture);
}

void MVulkanEngine::recordCommandBuffer(
    uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList,
    std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount)
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
    viewport.y = (float)extent.height;
    viewport.width = (float)extent.width;
    viewport.height = -(float)extent.height;
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
