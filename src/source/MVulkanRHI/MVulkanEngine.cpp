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

    createCamera();
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

void MVulkanEngine::Run()
{
    //glm::vec2 mousePos = window->GetMousePosition();
    ////glfwGetCursorPos(window, &mousePos.x, &mousePos.y);
    //Singleton<InputManager>::instance().SetMousePositionFirstTime(mousePos);
    while (!window->WindowShouldClose()) {
        renderLoop();
    }
}

void MVulkanEngine::drawFrame()
{
    inFlightFences[currentFrame].WaitForSignal(device.GetDevice());

    uint32_t imageIndex;
    VkResult result = swapChain.AcquireNextImage(device.GetDevice(), imageAvailableSemaphores[currentFrame].GetSemaphore(), VK_NULL_HANDLE, &imageIndex);
    //spdlog::info("imageIndex:" + std::to_string(imageIndex));

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

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
    for (auto i = 0; i < gbufferPass->GetFrameBuffer(0).ColorAttachmentsCount(); i++) {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = gbufferPass->GetFrameBuffer(0).GetImage(i);
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

    graphicsQueue.SubmitCommands(1, &submitInfo, inFlightFences[currentFrame].GetFence());
    graphicsQueue.WaitForQueueComplete();
    //done draw gbuffer

    barriers.clear();
    
    for (auto i = 0; i < gbufferPass->GetFrameBuffer(0).ColorAttachmentsCount(); i++) {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = gbufferPass->GetFrameBuffer(0).GetImage(i);
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

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
        transferList.TransitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
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
   

    transferList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();
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
    generateMeshDecriptorSets();
}

void MVulkanEngine::renderLoop()
{
    window->WindowPollEvents();
    Singleton<InputManager>::instance().DealInputs();
    drawFrame();
    //spdlog::info("****************************************");
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

        gbufferPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        lightingPass->TransitionFrameBufferImageLayout(transferQueue, transferList, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        std::vector<VkImageView> gbufferTextures(0);
        std::vector<VkImageView> lightingTextures(2);
        lightingTextures[0] = gbufferPass->GetFrameBuffer(0).GetImageView(0);
        lightingTextures[1] = gbufferPass->GetFrameBuffer(0).GetImageView(1);
        gbufferPass->UpdateDescriptorSetWrite(gbufferTextures);
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

    RenderPassCreateInfo info{};
    info.frambufferCount = 1;
    info.extent = swapChain.GetSwapChainExtent();
    info.useAttachmentResolve = false;
    info.useSwapchainImages = false;
    info.imageAttachmentFormats = gbufferFormats;
    info.colorAttachmentResolvedViews = nullptr;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    gbufferPass = std::make_shared<RenderPass>(device, info);

    std::shared_ptr<ShaderModule> gbufferShader = std::make_shared<GbufferShader>();
    std::vector<std::vector<VkImageView>> bufferTextureViews(1);
    for (auto i = 0; i < bufferTextureViews.size(); i++) {
        bufferTextureViews[i].resize(1);
        bufferTextureViews[i] = std::vector<VkImageView>(1);
        bufferTextureViews[i][0] = Singleton<TextureManager>::instance().Get("placeholder")->GetImageView();
        //bufferTextureViews[i][0] = 
    }

    gbufferPass->Create(gbufferShader, swapChain, graphicsQueue, generalGraphicList, allocator, bufferTextureViews);

    std::vector<VkFormat> lightingPassFormats;
    lightingPassFormats.push_back(swapChain.GetSwapChainImageFormat());

    info.frambufferCount = swapChain.GetImageCount();
    info.useSwapchainImages = true;
    info.imageAttachmentFormats = lightingPassFormats;
    info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ;

    lightingPass = std::make_shared<RenderPass>(device, info);

    std::shared_ptr<ShaderModule> lightingShader = std::make_shared<SquadPhongShader>();
    std::vector<std::vector<VkImageView>> gbufferViews(swapChain.GetImageCount());
    for (auto i = 0; i < swapChain.GetImageCount(); i++) {
        gbufferViews[i].push_back(gbufferPass->GetFrameBuffer(0).GetImageView(0));
        gbufferViews[i].push_back(gbufferPass->GetFrameBuffer(0).GetImageView(1));
        gbufferViews[i].push_back(gbufferPass->GetFrameBuffer(0).GetImageView(2));
    }

    lightingPass->Create(lightingShader, swapChain, graphicsQueue, generalGraphicList, allocator, gbufferViews);
}

void MVulkanEngine::generateMeshDecriptorSets()
{
    auto meshNames = scene->GetMeshNames();

    for (auto item : meshNames) {
        auto mesh = scene->GetMesh(item);
        mesh->descriptorSet = gbufferPass->CreateDescriptorSet(allocator);
    }
}

//void MVulkanEngine::resolveDescriptorSet()
//{
//
//}

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
    //loadModel();
    loadScene();

    squadVertexBuffer = Buffer(BufferType::VERTEX_BUFFER);
    squadIndexBuffer = Buffer(BufferType::INDEX_BUFFER);

    BufferCreateInfo vertexInfo;
    //vertexInfo.size = model->VertexSize();
    
    BufferCreateInfo indexInfo;
    //indexInfo.size = model->IndexSize();
    //
    transferList.Reset();
    transferList.Begin();
    //vertexBuffer.CreateAndLoadData(&transferList, device, vertexInfo, (void*)(model->GetVerticesData()));
    //indexBuffer.CreateAndLoadData(&transferList, device, indexInfo, (void*)model->GetIndicesData());
    //
    vertexInfo.size = sizeof(sqad_vertices);
    indexInfo.size = sizeof(sqad_indices);
    squadVertexBuffer.CreateAndLoadData(&transferList, device, vertexInfo, (void*)(sqad_vertices));
    squadIndexBuffer.CreateAndLoadData(&transferList, device, indexInfo, (void*)(sqad_indices));
    //
    transferList.End();
    //
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

void MVulkanEngine::loadScene()
{
    scene = std::make_shared<Scene>();

    fs::path resourcePath = fs::current_path().parent_path().parent_path().append("resources").append("models");
    //fs::path modelPath = resourcePath / "suzanne.obj";
    //fs::path modelPath = resourcePath / "CornellBox.glb";
    fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";

    Singleton<SceneLoader>::instance().Load(modelPath.string(), scene.get());
    //model->Load(modelPath.string());
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
    renderPassInfo.renderPass = lightingPass->GetRenderPass().Get();
    renderPassInfo.framebuffer = lightingPass->GetFrameBuffer(imageIndex).Get();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    graphicsLists[currentFrame].BeginRenderPass(&renderPassInfo);

    graphicsLists[currentFrame].BindPipeline(lightingPass->GetPipeline().Get());

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

    DirectionalLight light(glm::normalize(glm::vec3(-1.f, -1.f, -1.f)), glm::vec3(1.f), 1.f);
    
    SquadPhongShader::DirectionalLightBuffer ubo0{};
    ubo0.lightNum = 1;
    ubo0.lights[0].direction = light.GetDirection();
    ubo0.lights[0].intensity = light.GetIntensity();
    ubo0.lights[0].color = light.GetColor();

    ubo0.cameraPos = camera->GetPosition();

    lightingPass->GetShader()->SetUBO(0, &ubo0);
    lightingPass->LoadCBV();

    auto descriptorSets = lightingPass->GetDescriptorSet().Get();
    graphicsLists[currentFrame].BindDescriptorSet(lightingPass->GetPipeline().GetLayout(), 0, 1, &descriptorSets[imageIndex]);

    VkBuffer vertexBuffers[] = { squadVertexBuffer.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };

    graphicsLists[currentFrame].BindVertexBuffers(0, 1, vertexBuffers, offsets);
    graphicsLists[currentFrame].BindIndexBuffers(0, 1, squadIndexBuffer.GetBuffer(), offsets);

    //auto descriptorSets = lightingPass->GetDescriptorSet().Get();
    //graphicsLists[currentFrame].BindDescriptorSet(lightingPass->GetPipeline().GetLayout(), 0, 1, &descriptorSets[imageIndex]);

    graphicsLists[currentFrame].DrawIndexed(6, 1, 0, 0, 0);
    graphicsLists[currentFrame].EndRenderPass();
    graphicsLists[currentFrame].End();
}

void MVulkanEngine::renderPass(uint32_t currentFrame, uint32_t imageIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) {
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

//template<class T>
//void MVulkanEngine::CreateImage(std::shared_ptr<MVulkanTexture> texture, VkExtent2D extent, VkFormat format, MImage<T>* imageData)
//{
//    ImageCreateInfo imageInfo{};
//    imageInfo.width = extent.width;
//    imageInfo.height = extent.height;
//    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
//    imageInfo.format = format;
//    imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//
//    ImageViewCreateInfo viewInfo{};
//    viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
//    viewInfo.format = format;
//    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//    //viewInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    viewInfo.baseMipLevel = 0;
//    viewInfo.levelCount = 1;
//    viewInfo.baseArrayLayer = 0;
//    viewInfo.layerCount = 1;
//    viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
//
//    transferList.Reset();
//    transferList.Begin();
//    texture->CreateAndLoadData(&transferList, device, imageInfo, viewInfo, imageData);
//    transferList.End();
//
//    VkSubmitInfo submitInfo{};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submitInfo.commandBufferCount = 1;
//    submitInfo.pCommandBuffers = &transferList.GetBuffer();
//
//    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
//    transferQueue.WaitForQueueComplete();
//}


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

void MVulkanEngine::recordGbufferCommandBuffer(uint32_t imageIndex)
{
    graphicsLists[currentFrame].Begin();

    VkExtent2D swapChainExtent = swapChain.GetSwapChainExtent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = gbufferPass->GetRenderPass().Get();
    renderPassInfo.framebuffer = gbufferPass->GetFrameBuffer(0).Get();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 4> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[3].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    graphicsLists[currentFrame].BeginRenderPass(&renderPassInfo);

    graphicsLists[currentFrame].BindPipeline(gbufferPass->GetPipeline().Get());

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

    //VkBuffer vertexBuffers[] = { vertexBuffer.GetBuffer()};
    //VkDeviceSize offsets[] = { 0 };

    GbufferShader::UniformBufferObject ubo{};
    ubo.Model = glm::mat4(1.f);
    ubo.View = camera->GetViewMatrix();
    ubo.Projection = camera->GetProjMatrix();
    gbufferPass->GetShader()->SetUBO(0, &ubo);
    gbufferPass->LoadCBV();



    //auto descriptorSets = gbufferPass->GetDescriptorSet().Get();
    //graphicsLists[currentFrame].BindDescriptorSet(gbufferPass->GetPipeline().GetLayout(), 0, 1, &descriptorSets[0]);

    auto meshNames = scene->GetMeshNames();
    for (auto item : meshNames) {
        //spdlog::info("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
        std::vector<VkImageView> gbufferTextures(1);
        auto mat = scene->GetMaterial(scene->GetMesh(item)->matId);
        gbufferTextures[0] = Singleton<TextureManager>::instance().Get(mat->diffuseTexture)->GetImageView();
        //gbufferPass->UpdateDescriptorSetWrite(gbufferTextures);
        //graphicsLists[currentFrame].BindDescriptorSet(gbufferPass->GetPipeline().GetLayout(), 0, 1, &descriptorSets[0]);
        auto descriptorSets = scene->GetMesh(item)->descriptorSet.Get();
        auto descriptorSet = descriptorSets[0];
        gbufferPass->UpdateDescriptorSetWrite(scene->GetMesh(item)->descriptorSet, gbufferTextures);
        graphicsLists[currentFrame].BindDescriptorSet(gbufferPass->GetPipeline().GetLayout(), 0, 1, &descriptorSet);

        VkBuffer vertexBuffers[] = { scene->GetVertexBuffer(item)->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
    
        graphicsLists[currentFrame].BindVertexBuffers(0, 1, vertexBuffers, offsets);
        graphicsLists[currentFrame].BindIndexBuffers(0, 1, scene->GetIndexBuffer(item)->GetBuffer(), offsets);
    
        graphicsLists[currentFrame].DrawIndexed(static_cast<uint32_t>(scene->GetMesh(item)->indices.size()), 1, 0, 0, 0);
    }

    //VkBuffer vertexBuffers[] = { scene->GetIndirectVertexBuffer()->GetBuffer() };
    //VkDeviceSize offsets[] = { 0 };
    //
    //graphicsLists[currentFrame].BindVertexBuffers(0, 1, vertexBuffers, offsets);
    //graphicsLists[currentFrame].BindIndexBuffers(0, 1, scene->GetIndirectIndexBuffer()->GetBuffer(), offsets);
    //
    ////graphicsLists[currentFrame].DrawIndexed(static_cast<uint32_t>(scene->GetMesh(item)->indices.size()), 1, 0, 0, 0);
    //graphicsLists[currentFrame].DrawIndexedIndirectCommand(scene->GetIndirectBuffer()->GetBuffer(), 0, scene->GetIndirectDrawCommands().size(), sizeof(VkDrawIndexedIndirectCommand));

    graphicsLists[currentFrame].EndRenderPass();

    graphicsLists[currentFrame].End();
}

//std::vector<VkDescriptorBufferInfo> MVulkanEngine::generateDescriptorBufferInfos(std::vector<VkBuffer> buffers, std::vector<ShaderResourceInfo> resourceInfos)
//{
//    std::vector<VkDescriptorBufferInfo> bufferInfos;
//
//    for (auto i = 0; i < resourceInfos.size(); i++) {
//        VkDescriptorBufferInfo bufferInfo{};
//        bufferInfo.buffer = buffers[i];
//        bufferInfo.offset = resourceInfos[i].offset;
//        bufferInfo.range = resourceInfos[i].size;
//
//        bufferInfos.push_back(bufferInfo);
//    }
//
//    return bufferInfos;
//}
