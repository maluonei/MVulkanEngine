#pragma once
#ifndef MVULKANENGINE_H
#define MVULKANENGINE_H

#include <cstdint>
#include <vulkan\vulkan_core.h>
#include "MVulkanInstance.hpp"
#include "MVulkanDevice.hpp"
#include "MVulkanDescriptor.hpp"
#include "MVulkanSwapchain.hpp"
#include "MVulkanPipeline.hpp"
#include "MVulkanRenderPass.hpp"
#include "MVulkanFrameBuffer.hpp"
#include "MVulkanCommand.hpp"
#include "MVulkanBuffer.hpp"
#include "MVulkanSampler.hpp"
#include "MVulkanAttachmentBuffer.hpp"
#include "Shaders/ShaderResourceMap.hpp"
#include "Shaders/ShaderModule.hpp"
#include "MVulkanShader.hpp"
#include "Utils/MImage.hpp"
#include <memory>
#include "MVulkanSyncObject.hpp"
#include "Managers/Singleton.hpp"
//#include "Camera.hpp"

#define MAX_FRAMES_IN_FLIGHT 2

class MWindow;
class Camera;
class Model;
class RenderPass;

class MVulkanEngine: public Singleton<MVulkanEngine> {
public:
    MVulkanEngine();
    ~MVulkanEngine();

    void Init();
    void Clean();

    void Run();
    void SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight);

    void GenerateMipMap(MVulkanTexture texture);

    MVulkanSampler GetGlobalSampler()const;
private:
    void initVulkan();
    void renderLoop();
    void drawFrame();

    void createGlobalSamplers();

    void createInstance();
    void createDevice();
    void createSurface();
    void createSwapChain();
    void RecreateSwapChain();
    void transitionSwapchainImageFormat();
    void transitionFramebufferImageLayout();

    void createRenderPass();

    void createDescriptorSetAllocator();

    void createCommandQueue();
    void createCommandAllocator();
    void createCommandList();
    void createCamera();

    void createBufferAndLoadData();
    void createTexture();
    void createSampler();
    void loadModel();
    
    void createSyncObjects();
    void recordGbufferCommandBuffer(uint32_t imageIndex);
    void recordFinalCommandBuffer(uint32_t imageIndex);

    void renderPass(uint32_t currentFrame, uint32_t imageIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore);

    VkSubmitInfo generateSubmitInfo(std::vector<VkCommandBuffer> commandBuffer, std::vector<VkSemaphore> waitSemaphores, std::vector<VkPipelineStageFlags> waitSemaphoreStage, std::vector<VkSemaphore> signalSemaphores);
    VkSubmitInfo generateSubmitInfo(VkCommandBuffer* commandBuffer, VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitSemaphoreStage, VkSemaphore* signalSemaphores);
    void submitCommand();
    void transitionImageLayout(MVulkanImageMemoryBarrier barrier, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage,
        VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitSemaphoreStage, VkSemaphore* signalSemaphores, VkFence fence = VK_NULL_HANDLE);

    void transitionImageLayouts(
        std::vector<MVulkanImageMemoryBarrier> barrier, 
        VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage,
        VkSemaphore* waitSemaphores, VkPipelineStageFlags* waitSemaphoreStage, VkSemaphore* signalSemaphores, VkFence fence = VK_NULL_HANDLE);

    void present(VkSwapchainKHR* swapChains, VkSemaphore* waitSemaphore, const uint32_t* imageIndex);

    std::vector<VkDescriptorBufferInfo> generateDescriptorBufferInfos(std::vector<VkBuffer> buffers, std::vector<ShaderResourceInfo> resourceInfos);

private:
    std::shared_ptr<RenderPass> gbufferPass;
    std::shared_ptr<RenderPass> lightingPass;

    uint16_t windowWidth = 800, windowHeight = 600;

    MWindow* window = nullptr;

    bool enableValidationLayer;
    
    MVulkanInstance instance;
    MVulkanDevice device;
    MVulkanSwapchain swapChain;

    MVulkanDescriptorSetAllocator allocator;
    //MVulkanDescriptorSetLayouts layouts;
    //MVulkanDescriptorSet descriptorSet;
    
    VkSurfaceKHR surface;
    MVulkanCommandQueue graphicsQueue;
    MVulkanCommandQueue presentQueue;
    MVulkanCommandQueue transferQueue;
    MVulkanCommandAllocator commandAllocator;

    std::vector<MGraphicsCommandList> graphicsLists;
    MGraphicsCommandList generalGraphicList;
    MGraphicsCommandList presentList;
    MGraphicsCommandList transferList;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    VertexBuffer sqadVertexBuffer;
    IndexBuffer sqadIndexBuffer;

    MVulkanSampler sampler;
    MVulkanTexture testTexture;
    MImage<unsigned char> image;

    std::vector<MVulkanSemaphore> imageAvailableSemaphores;
    std::vector<MVulkanSemaphore> renderFinishedSemaphores;
    std::vector<MVulkanSemaphore> gbufferRenderFinishedSemaphores;
    std::vector<MVulkanSemaphore> finalRenderFinishedSemaphores;
    std::vector<MVulkanSemaphore> gbufferTransferFinishedSemaphores;
    std::vector<MVulkanSemaphore> transferFinishedSemaphores;
    std::vector<MVulkanFence> inFlightFences;
    uint32_t currentFrame = 0;

    std::shared_ptr<Camera> camera;
    std::shared_ptr<Model> model;

    std::vector<MVulkanSampler> globalSamplers;
};

#endif