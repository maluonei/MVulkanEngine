#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"MVulkanRHI/MWindow.hpp"
#include"MVulkanRHI/MVulkanEngine.hpp"
#include <stdexcept>

#include "MVulkanRHI/MVulkanShader.hpp"

float verteices[] = {
    0.8f, 0.8f, 0.f,   0.f, 0.f, 1.f,   1.f, 1.f,
    -0.8f, -0.8f, 0.f, 0.f, 0.f, 1.f,   0.f, 0.f,
    0.8f, -0.8f, 0.f,  0.f, 0.f, 1.f,   1.f, 0.f,
    -0.8f, 0.8f, 0.f,  0.f, 0.f, 1.f,   0.f, 1.f,
};

//float verteices2[] = {
//    1.f, 1.f, 0.f,   
//    -1.f, -1.f, 0.f, 
//    1.f, -1.f, 0.f,  
//    -1.f, 1.f, 0.f,  
//};

uint32_t indices[] = {
    0, 1, 2,
    0, 3, 1
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
    VkResult result = swapChain.AcquireNextImage(device.GetDevice(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

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

    result = presentQueue.Present(&presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window->GetWindowResized()) {
        window->SetWindowResized(false);
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

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

    createDescriptorSetAllocator();

    createFrameBuffers();
    createCommandQueue();
    createCommandAllocator();
    createCommandList();
    createSyncObjects();

    createBufferAndLoadData();

    createPipeline();
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
}

void MVulkanEngine::RecreateSwapChain()
{

    if (swapChain.Recreate(device, window->GetWindow(), surface)) {
        
        for (auto frameBuffer : frameBuffers) {
            frameBuffer.Clean(device.GetDevice());
        }

        createFrameBuffers();
    }
}

void MVulkanEngine::createRenderPass()
{
    renderPass.Create(device.GetDevice(), swapChain.GetSwapChainImageFormat());
}

void MVulkanEngine::resolveDescriptorSet()
{

}

void MVulkanEngine::createDescriptorSetAllocator()
{
    allocator.Create(device.GetDevice());
}

void MVulkanEngine::createPipeline()
{
    std::string vertPath = "test.vert.glsl";
    std::string fragPath = "test.frag.glsl";

    MVulkanShader vert(vertPath, ShaderStageFlagBits::VERTEX);
    MVulkanShader frag(fragPath, ShaderStageFlagBits::FRAGMENT);

    vert.Create(device.GetDevice());
    frag.Create(device.GetDevice());

    MVulkanShaderReflector vertReflector(vert.GetShader());
    MVulkanShaderReflector fragReflector(frag.GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();
    //fragReflector.GenerateDescriptorSet();

    ShaderReflectorOut resourceOut = fragReflector.GenerateShaderReflactorOut();
    std::vector<VkDescriptorSetLayoutBinding> bindings = resourceOut.GetBindings();

    layouts.Create(device.GetDevice(), bindings);
    descriptorSet.Create(device.GetDevice(), allocator.Get(), layouts.Get(), MAX_FRAMES_IN_FLIGHT);

    glm::vec3 color0 = glm::vec3(1.f, 0.f, 0.f);
    glm::vec3 color1 = glm::vec3(0.f, 1.f, 0.f);
    testFrag.SetUBO(color0, color1);
    
    cbvs.resize(MAX_FRAMES_IN_FLIGHT);
    BufferCreateInfo _info;
    _info.size = testFrag.GetBufferSize();

    //transferList.Reset();
    //transferList.Begin();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        //cbvs[i].CreateAndLoadData(&transferList, device, _info, testFrag.GetData());
        cbvs[i].Create(device, _info);
    }

    createTexture();
    createSampler();

    //fs::path resourcePath = "F:/MVulkanEngine/resources/textures";
    //fs::path imagePath = resourcePath / "test.jpg";
    //image.Load(imagePath);
    //
    //ImageCreateInfo imageCreateInfo;
    //imageCreateInfo.width = image.Width();
    //imageCreateInfo.height = image.Height();
    //imageCreateInfo.format = image.Format();
    //imageCreateInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    //imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    //
    //testTexture.CreateAndLoadData(&transferList, device, imageCreateInfo, &image);
    //transferList.End();
    
    MVulkanDescriptorSetWrite write;

    std::vector<VkDescriptorBufferInfo> bufferInfos(1);
    for (int i = 0; i < 1; i++) {
        bufferInfos[i].buffer = cbvs[i].GetBuffer();
        bufferInfos[i].offset = 0;
        bufferInfos[i].range = testFrag.GetBufferSize();
    }

    std::vector<VkDescriptorImageInfo> imageInfos(1);
    for (int i = 0; i < 1; i++) {
        imageInfos[i].sampler = sampler.GetSampler();
        imageInfos[i].imageView = testTexture.GetImageView();
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    write.Update(device.GetDevice(), descriptorSet.Get(), bufferInfos, imageInfos, MAX_FRAMES_IN_FLIGHT);

    pipeline.Create(device.GetDevice(), vert.GetShaderModule(), frag.GetShaderModule(), renderPass.Get(), info, layouts.Get());

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

    shaderList.Create(device.GetDevice(), info);

    info.commandPool = commandAllocator.Get(QueueType::TRANSFER_QUEUE);
    transferList.Create(device.GetDevice(), info);

    info.commandPool = commandAllocator.Get(QueueType::PRESENT_QUEUE);
    presentList.Create(device.GetDevice(), info);
}

void MVulkanEngine::createBufferAndLoadData()
{
    BufferCreateInfo vertexInfo;
    vertexInfo.size = sizeof(verteices);

    BufferCreateInfo indexInfo;
    indexInfo.size = sizeof(indices);
    
    transferList.Reset();
    transferList.Begin();
    vertexBuffer.CreateAndLoadData(&transferList, device, vertexInfo, verteices);
    indexBuffer.CreateAndLoadData(&transferList, device, indexInfo, indices);
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
    fs::path resourcePath = "F:/MVulkanEngine/resources/textures";
    fs::path imagePath = resourcePath / "texture.jpg";
    image.Load(imagePath);

    ImageCreateInfo info;
    info.width = image.Width();
    info.height = image.Height();
    info.format = image.Format();
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;

    shaderList.Reset();
    shaderList.Begin();
    testTexture.CreateAndLoadData(&shaderList, device, info, &image);
    shaderList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &shaderList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();
    ///Submit
    //testTexture.Create(device, info);
}

void MVulkanEngine::createSampler()
{
    sampler.Create(device);
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
    graphicsLists[currentFrame].Begin();

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

    graphicsLists[currentFrame].BeginRenderPass(&renderPassInfo);

    graphicsLists[currentFrame].BindPipeline(pipeline.Get());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
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

    auto descriptorSets = descriptorSet.Get();
    graphicsLists[currentFrame].BindDescriptorSet(pipeline.GetLayout(), 0, 1, &descriptorSets[currentFrame]);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        cbvs[i].UpdateData(device, testFrag.GetData());
    }

    graphicsLists[currentFrame].DrawIndexed(6, 1, 0, 0, 0);

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


