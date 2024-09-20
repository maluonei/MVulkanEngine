#pragma once
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
#ifndef MVULKANENGINE_H
#define MVULKANENGINE_H

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

    void createInstance();
    void createDevice();
    void createSurface();
    void createSwapChain();
    void createRenderPass();
    void createPipeline();
    void createFrameBuffers();
    void createCommandQueue();
    void createCommandAllocator();
    void createCommandList();
    void createBufferAndLoadData();

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
    MVulkanCommandAllocator commandAllocator;
    MVulkanCommandList commandList;

    ShaderInputBuffer vertexBuffer;
    ShaderInputBuffer indexBuffer;

    VkFence fence;

    //VkInstance instance;
};

#endif