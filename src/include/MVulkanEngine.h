#pragma once
#ifndef MVULKANENGINE_H
#define MVULKANENGINE_H

#include <cstdint>
#include <vulkan\vulkan_core.h>
#include "MVulkanInstance.h"
#include "MVulkanDevice.h"
#include "MVulkanSwapchain.h"
#include "MVulkanPipeline.h"
#include "MVulkanRenderPass.h"
#include "MVulkanFrameBuffer.h"
#include "MVulkanCommand.h"
#include "MVulkanBuffer.h"

#define MAX_FRAMES_IN_FLIGHT 2

class MWindow;

class MVulkanEngine {
public:
    MVulkanEngine();
    ~MVulkanEngine();

    void Init();
    void Clean();

    void Run();
    void SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight);
private:
    void initVulkan();
    void renderLoop();
    void drawFrame();

    void createInstance();
    void createDevice();
    void createSurface();
    void createSwapChain();
    void RecreateSwapChain();
    void createRenderPass();
    void createPipeline();
    void createFrameBuffers();
    void createCommandQueue();
    void createCommandAllocator();
    void createCommandList();
    void createBufferAndLoadData();
    void createSyncObjects();
    void recordCommandBuffer(uint32_t imageIndex);

    uint16_t windowWidth = 800, windowHeight = 600;

    MWindow* window = nullptr;

    bool enableValidationLayer;
    
    MVulkanInstance instance;
    MVulkanDevice device;
    MVulkanSwapchain swapChain;
    MVulkanGraphicsPipeline pipeline;
    MVulkanrRenderPass renderPass;
    std::vector<MVulkanFrameBuffer> frameBuffers;
    VkSurfaceKHR surface;
    MVulkanCommandQueue graphicsQueue;
    MVulkanCommandQueue presentQueue;
    MVulkanCommandQueue transferQueue;
    MVulkanCommandAllocator commandAllocator;
    std::vector<MGraphicsCommandList> graphicsLists;
    MGraphicsCommandList presentList;
    MGraphicsCommandList transferList;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    //bool framebufferResized = false;

    //VkInstance instance;

    //VkBuffer vertexBuffer;
    //VkDeviceMemory vertexBufferMemory;
};

#endif