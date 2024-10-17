#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"MVulkanRHI/MWindow.hpp"
#include"MVulkanRHI/MVulkanEngine.hpp"
#include <stdexcept>
#include "Camera.hpp"
#include "Scene/Model.hpp"

#include "MVulkanRHI/MVulkanShader.hpp"

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

//float verteices2[] = {
//    1.f, 1.f, 0.f,   
//    -1.f, -1.f, 0.f, 
//    1.f, -1.f, 0.f,  
//    -1.f, 1.f, 0.f,  
//};

uint32_t indices[] = {
    0, 1, 2,
    0, 3, 1,
    4, 5, 6,
    4, 7, 5
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

    transferList.Reset();
    transferList.Begin();
    //spdlog::info("a");
    //transferList.TransitionImageLayout(frameBuffers[imageIndex].GetImage(0), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    //spdlog::info("b");
    //transferList.TransitionImageLayout(swapChain.GetImage(imageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    transferList.TransitionImageLayout(swapChain.GetImage(imageIndex), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //spdlog::info("c");
    transferList.CopyImage(frameBuffers[imageIndex].GetImage(0), swapChain.GetImage(imageIndex), swapChain.GetSwapChainExtent().width, swapChain.GetSwapChainExtent().height);
    //spdlog::info("d");
    //transferList.TransitionImageLayout(frameBuffers[imageIndex].GetImage(0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    transferList.TransitionImageLayout(swapChain.GetImage(imageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    //spdlog::info("e");
    transferList.End();
 
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferList.GetBuffer();

    VkSemaphore waitSemaphores0[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages0[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores0;
    submitInfo.pWaitDstStageMask = waitStages0;

    VkSemaphore signalSemaphores0[] = { transferFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores0;

    //spdlog::info("f");
    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    transferQueue.WaitForQueueComplete();

    //VkSubmitInfo submitInfo{};
    //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores1[] = { transferFinishedSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages1[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores1;
    submitInfo.pWaitDstStageMask = waitStages1;

    //spdlog::info("g");

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsLists[currentFrame].GetBuffer();

    VkSemaphore signalSemaphores1[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores1;

    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue.Get(), 1, &submitInfo, inFlightFences[currentFrame]));

    //spdlog::info("h");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores1;

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

    createCommandQueue();
    createCommandAllocator();
    createCommandList();
    createSyncObjects();

    createSwapChain();

    createDescriptorSetAllocator();

    //createDepthBuffer();
    createRenderPass();
    createFrameBuffers();

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

    transitionSwapchainImageFormat();
}

void MVulkanEngine::RecreateSwapChain()
{

    if (swapChain.Recreate(device, window->GetWindow(), surface)) {
        transitionSwapchainImageFormat();

        for (auto frameBuffer : frameBuffers) {
            frameBuffer.Clean(device.GetDevice());
        }

        createFrameBuffers();
    }
}

void MVulkanEngine::transitionSwapchainImageFormat()
{
    transferList.Reset();
    transferList.Begin();

    std::vector<VkImage> images = swapChain.GetSwapChainImages();
    for (auto i = 0; i < images.size(); i++) {
        transferList.TransitionImageLayout(images[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

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
    renderPass.Create(device.GetDevice(), swapChain.GetSwapChainImageFormat(), device.FindDepthFormat());
}

//void MVulkanEngine::resolveDescriptorSet()
//{
//
//}

void MVulkanEngine::createDescriptorSetAllocator()
{
    allocator.Create(device.GetDevice());
}

void MVulkanEngine::createPipeline()
{
    std::string vertPath = "phong.vert.glsl";
    std::string fragPath = "phong.frag.glsl";

    MVulkanShader vert(vertPath, ShaderStageFlagBits::VERTEX);
    MVulkanShader frag(fragPath, ShaderStageFlagBits::FRAGMENT);

    vert.Create(device.GetDevice());
    frag.Create(device.GetDevice());

    MVulkanShaderReflector vertReflector(vert.GetShader());
    MVulkanShaderReflector fragReflector(frag.GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();
    //fragReflector.GenerateDescriptorSet();

    ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<VkDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    std::vector<VkDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());

    layouts.Create(device.GetDevice(), bindings);
    descriptorSet.Create(device.GetDevice(), allocator.Get(), layouts.Get(), MAX_FRAMES_IN_FLIGHT);

    //glm::vec3 color0 = glm::vec3(1.f, 0.f, 0.f);
    //glm::vec3 color1 = glm::vec3(0.f, 1.f, 0.f);
    phongShader.SetUBO0(glm::mat4(1.f), camera->GetViewMatrix(), camera->GetProjMatrix());
    phongShader.SetUBO1(camera->GetPosition());
    
    cbvs0.resize(MAX_FRAMES_IN_FLIGHT);
    cbvs1.resize(MAX_FRAMES_IN_FLIGHT);
    BufferCreateInfo _infoUBO0;
    _infoUBO0.size = phongShader.GetBufferSizeBinding(0);
    BufferCreateInfo _infoUBO1;
    _infoUBO1.size = phongShader.GetBufferSizeBinding(1);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        cbvs0[i].Create(device, _infoUBO0);
        cbvs1[i].Create(device, _infoUBO1);
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

    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        bufferInfos[i].resize(2);

        bufferInfos[i][0].buffer = cbvs0[i].GetBuffer();
        bufferInfos[i][0].offset = 0;
        bufferInfos[i][0].range = phongShader.GetBufferSizeBinding(0);

        bufferInfos[i][1].buffer = cbvs1[i].GetBuffer();
        bufferInfos[i][1].offset = 0;
        bufferInfos[i][1].range = phongShader.GetBufferSizeBinding(1);
    }

    std::vector < std::vector<VkDescriptorImageInfo>> imageInfos(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        imageInfos[i].resize(1);

        imageInfos[i][0].sampler = sampler.GetSampler();
        imageInfos[i][0].imageView = testTexture.GetImageView();
        imageInfos[i][0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    write.Update(device.GetDevice(), descriptorSet.Get(), bufferInfos, imageInfos, MAX_FRAMES_IN_FLIGHT);

    pipeline.Create(device.GetDevice(), vert.GetShaderModule(), frag.GetShaderModule(), renderPass.Get(), info, layouts.Get());

    vert.Clean(device.GetDevice());
    frag.Clean(device.GetDevice());
}

//void MVulkanEngine::createDepthBuffer()
//{
//    VkExtent2D extent = swapChain.GetSwapChainExtent(); 
//    VkFormat format = device.FindDepthFormat();
//
//    depthBuffer.Create(device, extent, format);
//}

void MVulkanEngine::createFrameBuffers()
{
    std::vector<VkImageView> imageViews = swapChain.GetSwapChainImageViews();
    //VkImageView depthImageView = depthBuffer.GetImageView();

    transferList.Reset();
    transferList.Begin();

    frameBuffers.resize(imageViews.size());
    for (auto i = 0; i < imageViews.size(); i++) {
        std::vector<VkFormat> formats(1);
        formats[0] = swapChain.GetSwapChainImageFormat();

        FrameBufferCreateInfo info{};

        info.renderPass = renderPass.Get();
        info.extent = swapChain.GetSwapChainExtent();
        info.imageAttachmentFormats = formats.data();
        info.numAttachments = static_cast<uint32_t>(formats.size());

        frameBuffers[i].Create(device, info);

        transferList.TransitionImageLayout(frameBuffers[i].GetImage(0), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
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

    shaderList.Create(device.GetDevice(), info);

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
    //vertexInfo.size = sizeof(verteices);
    vertexInfo.size = model->VertexSize();

    BufferCreateInfo indexInfo;
    //indexInfo.size = sizeof(indices);
    indexInfo.size = model->IndexSize();
    
    transferList.Reset();
    transferList.Begin();
    //vertexBuffer.CreateAndLoadData(&transferList, device, vertexInfo, verteices);
    //indexBuffer.CreateAndLoadData(&transferList, device, indexInfo, indices)
    vertexBuffer.CreateAndLoadData(&transferList, device, vertexInfo, (void*)(model->GetVerticesData()));
    indexBuffer.CreateAndLoadData(&transferList, device, indexInfo, (void*)model->GetIndicesData());
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

    shaderList.Reset();
    shaderList.Begin();
    testTexture.CreateAndLoadData(&shaderList, device, imageinfo, viewInfo, &image);
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
    transferFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, nullptr, &transferFinishedSemaphores[i]) != VK_SUCCESS ||
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

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    graphicsLists[currentFrame].BeginRenderPass(&renderPassInfo);

    graphicsLists[currentFrame].BindPipeline(pipeline.Get());

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

    auto descriptorSets = descriptorSet.Get();
    graphicsLists[currentFrame].BindDescriptorSet(pipeline.GetLayout(), 0, 1, &descriptorSets[currentFrame]);

    phongShader.SetUBO0(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f)), camera->GetViewMatrix(), camera->GetProjMatrix());
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        cbvs0[i].UpdateData(device, phongShader.GetData(0));
        cbvs1[i].UpdateData(device, phongShader.GetData(1));
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


