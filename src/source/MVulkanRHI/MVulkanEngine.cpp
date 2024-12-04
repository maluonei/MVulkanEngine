#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"MVulkanRHI/MWindow.hpp"
#include"MVulkanRHI/MVulkanEngine.hpp"
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

    createLight();
    createCamera();
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
    //spdlog::info("a");
    inFlightFences[currentFrame].WaitForSignal(device.GetDevice());

    uint32_t imageIndex;
    VkResult result = swapChain.AcquireNextImage(device.GetDevice(), imageAvailableSemaphores[0].GetSemaphore(), VK_NULL_HANDLE, &imageIndex);
    //spdlog::info("imageIndex:" + std::to_string(imageIndex));

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
            auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
            auto indirectCommand = drawIndexedIndirectCommands[i];
            ubo1[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
            auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
            ubo1[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = metallicAndRoughnessTexId;
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
        ubo0.ResolusionWidth = lightingPass->GetFrameBuffer(0).GetExtent2D().width;
        ubo0.ResolusionHeight = lightingPass->GetFrameBuffer(0).GetExtent2D().height;

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

        //std::vector<std::vector<VkImageView>> gbufferViews(5);
        //for (auto i = 0; i < 4; i++) {
        //    gbufferViews[i].resize(1);
        //    gbufferViews[i][0] = gbufferPass->GetFrameBuffer(0).GetImageView(i);
        //}
        //gbufferViews[4] = std::vector<VkImageView>(2);
        //gbufferViews[4][0] = shadowPass->GetFrameBuffer(0).GetDepthImageView();
        //gbufferViews[4][1] = shadowPass->GetFrameBuffer(0).GetDepthImageView();
        //lightingPass->UpdateDescriptorSetWrite(gbufferViews);

        //spdlog::info("sizeof(UniformBuffer0):{}", sizeof(LightingPbrShader::UniformBuffer0));
        //spdlog::info("sizeof(DirectionalLightBuffer):{}", sizeof(LightingPbrShader::DirectionalLightBuffer));
    }

    graphicsLists[currentFrame].Reset();
    graphicsLists[currentFrame].Begin();

    recordCommandBuffer(0, gbufferPass, scene->GetIndirectVertexBuffer(), scene->GetIndirectIndexBuffer(), scene->GetIndirectBuffer(), scene->GetIndirectDrawCommands().size());
    recordCommandBuffer(0, shadowPass,  scene->GetIndirectVertexBuffer(), scene->GetIndirectIndexBuffer(), scene->GetIndirectBuffer(), scene->GetIndirectDrawCommands().size());
    recordCommandBuffer(imageIndex, lightingPass, squadVertexBuffer, squadIndexBuffer, lightPassIndirectBuffer, 1);

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
        lightingPass->RecreateFrameBuffers(swapChain, transferQueue, transferList, swapChain.GetSwapChainExtent());
        shadowPass->RecreateFrameBuffers(swapChain, transferQueue, transferList, swapChain.GetSwapChainExtent());

        gbufferPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        lightingPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        shadowPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        std::vector<std::vector<VkImageView>> lightingTextures(4);
        for (auto i = 0; i < lightingPass->GetFramebufferCount(); i++) {
            lightingTextures[i].resize(1);
            lightingTextures[i][0] = gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        lightingTextures[3].resize(2);
        lightingTextures[3][0] = shadowPass->GetFrameBuffer(0).GetDepthImageView();
        lightingTextures[3][1] = shadowPass->GetFrameBuffer(0).GetDepthImageView();

        lightingPass->UpdateDescriptorSetWrite(lightingTextures);
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
    std::vector<VkFormat> gbufferFormats;
    gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
    gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
    gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
    gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

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
    info.depthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

    gbufferPass->Create(gbufferShader, swapChain, graphicsQueue, generalGraphicList, allocator, bufferTextureViews);

    std::vector<VkFormat> shadowFormats(0);
    shadowFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

    info.depthFormat = VK_FORMAT_D16_UNORM;
    info.frambufferCount = 1;
    info.extent = VkExtent2D(2048, 2048);
    info.useAttachmentResolve = false;
    info.useSwapchainImages = false;
    info.imageAttachmentFormats = shadowFormats;
    info.colorAttachmentResolvedViews = nullptr;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.depthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    shadowPass = std::make_shared<RenderPass>(device, info);

    std::shared_ptr<ShaderModule> shadowShader = std::make_shared<ShadowShader>();
    std::vector<std::vector<VkImageView>> shadowShaderTextures(1);
    shadowShaderTextures[0].resize(0);

    shadowPass->Create(shadowShader, swapChain, graphicsQueue, generalGraphicList, allocator, shadowShaderTextures);

    std::vector<VkFormat> lightingPassFormats;
    lightingPassFormats.push_back(swapChain.GetSwapChainImageFormat());

    info.extent = swapChain.GetSwapChainExtent();
    info.depthFormat = device.FindDepthFormat();
    info.frambufferCount = swapChain.GetImageCount();
    info.useSwapchainImages = true;
    info.imageAttachmentFormats = lightingPassFormats;
    info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ;
    info.depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    lightingPass = std::make_shared<RenderPass>(device, info);

    //std::shared_ptr<ShaderModule> lightingShader = std::make_shared<SquadPhongShader>();
    std::shared_ptr<ShaderModule> lightingShader = std::make_shared<LightingPbrShader>();
    std::vector<std::vector<VkImageView>> gbufferViews(5);
    for (auto i = 0; i < 4; i++) {
        gbufferViews[i].resize(1);
        gbufferViews[i][0] = gbufferPass->GetFrameBuffer(0).GetImageView(i);
    }
    gbufferViews[4] = std::vector<VkImageView>(2);
    gbufferViews[4][0] = shadowPass->GetFrameBuffer(0).GetDepthImageView();
    gbufferViews[4][1] = shadowPass->GetFrameBuffer(0).GetDepthImageView();

    lightingPass->Create(lightingShader, swapChain, graphicsQueue, generalGraphicList, allocator, gbufferViews);
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
    testTexture.CreateAndLoadData(&generalGraphicList, device, imageinfo, viewInfo, &image);
    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();
}

void MVulkanEngine::createSampler()
{
    sampler.Create(device);
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
}

void MVulkanEngine::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 100.f;
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
    CreateImage(texture, &tempImage);

    Singleton<TextureManager>::instance().Put("placeholder", texture);
}

void MVulkanEngine::recordCommandBuffer(
    uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, 
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
    graphicsLists[currentFrame].BeginRenderPass(&renderPassInfo);

    graphicsLists[currentFrame].BindPipeline(renderPass->GetPipeline().Get());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = (float)extent.height;
    viewport.width = (float)extent.width;
    viewport.height = -(float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    graphicsLists[currentFrame].SetViewport(0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    graphicsLists[currentFrame].SetScissor(0, 1, &scissor);

    renderPass->LoadCBV();

    auto descriptorSet = renderPass->GetDescriptorSet(imageIndex).Get();
    graphicsLists[currentFrame].BindDescriptorSet(renderPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);

    VkBuffer vertexBuffers[] = { vertexBuffer->GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    graphicsLists[currentFrame].BindVertexBuffers(0, 1, vertexBuffers, offsets);
    graphicsLists[currentFrame].BindIndexBuffers(0, 1, indexBuffer->GetBuffer(), offsets);

    graphicsLists[currentFrame].DrawIndexedIndirectCommand(indirectBuffer->GetBuffer(), 0, indirectCount, sizeof(VkDrawIndexedIndirectCommand));
    graphicsLists[currentFrame].EndRenderPass();
}
