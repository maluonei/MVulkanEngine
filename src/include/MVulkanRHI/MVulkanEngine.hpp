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
//#include "Camera.hpp"

#define MAX_FRAMES_IN_FLIGHT 2

class MWindow;
class Camera;
class Model;

class MVulkanEngine {
public:
    MVulkanEngine();
    ~MVulkanEngine();

    void Init();
    void Clean();

    void Run();
    void SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight);

    void GenerateMipMap(MVulkanTexture texture);
private:
    void initVulkan();
    void renderLoop();
    void drawFrame();

    void createInstance();
    void createDevice();
    void createSurface();
    void createSwapChain();
    void RecreateSwapChain();
    void transitionSwapchainImageFormat();
    void createGbufferRenderPass();
    void createRenderPass();
    //void resolveDescriptorSet();
    void createDescriptorSetAllocator();
    void createGbufferPipeline();
    void createSquadPipeline();
    void createPipeline();
    //void createDepthBuffer();
    void createGbufferFrameBuffers();
    void createFrameBuffers();
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

    std::vector<VkDescriptorBufferInfo> generateDescriptorBufferInfos(std::vector<VkBuffer> buffers, std::vector<ShaderResourceInfo> resourceInfos);

    uint16_t windowWidth = 800, windowHeight = 600;

    MWindow* window = nullptr;

    bool enableValidationLayer;
    
    MVulkanInstance instance;
    MVulkanDevice device;
    MVulkanSwapchain swapChain;

    //MVulkanGraphicsPipeline pipeline;
    //MVulkanRenderPass renderPass;
    //std::vector<MVulkanFrameBuffer> frameBuffers;
    MVulkanGraphicsPipeline gbufferPipeline;
    MVulkanGraphicsPipeline finalPipeline;
    MVulkanRenderPass gbufferRenderPass;
    MVulkanRenderPass finalRenderPass;
    std::vector<MVulkanFrameBuffer> gbufferFrameBuffers;
    std::vector<MVulkanFrameBuffer> finalFrameBuffers;

    MVulkanDescriptorSetAllocator allocator;
    MVulkanDescriptorSetLayouts layouts;
    MVulkanDescriptorSetLayouts gbufferDescriptorLayouts;
    MVulkanDescriptorSetLayouts finalDescriptorLayouts;
    MVulkanDescriptorSet descriptorSet;
    MVulkanDescriptorSet gbufferDescriptorSet;
    MVulkanDescriptorSet finalDescriptorSet;
    
    VkSurfaceKHR surface;
    MVulkanCommandQueue graphicsQueue;
    MVulkanCommandQueue presentQueue;
    MVulkanCommandQueue transferQueue;
    MVulkanCommandAllocator commandAllocator;

    std::vector<MGraphicsCommandList> graphicsLists;
    MGraphicsCommandList shaderList;
    MGraphicsCommandList presentList;
    MGraphicsCommandList transferList;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    VertexBuffer sqadVertexBuffer;
    IndexBuffer sqadIndexBuffer;
    std::vector<MCBV> gbufferCbvs0;
    std::vector<MCBV> cbvs0;
    std::vector<MCBV> cbvs1;
    MVulkanSampler sampler;
    MVulkanTexture testTexture;
    MImage<unsigned char> image;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> gbufferRenderFinishedSemaphores;
    std::vector<VkSemaphore> finalRenderFinishedSemaphores;
    std::vector<VkSemaphore> gbufferTransferFinishedSemaphores;
    std::vector<VkSemaphore> transferFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    //TestShader testShader;
    PhongShader phongShader;
    GbufferShader gbufferShader;
    SquadPhongShader squadPhongShader;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Model> model;
    //bool framebufferResized = false;

    //VkInstance instance;

    //VkBuffer vertexBuffer;
    //VkDeviceMemory vertexBufferMemory;
};

#endif