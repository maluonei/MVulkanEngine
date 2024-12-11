#pragma once
#ifndef MVULKANENGINE_HPP
#define MVULKANENGINE_HPP

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
class Scene;
class RenderPass;
class Light;

class MVulkanEngine: public Singleton<MVulkanEngine> {
public:
    MVulkanEngine();
    ~MVulkanEngine();

    void Init();
    void Clean();

    void Run();
    void SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight);

    void GenerateMipMap(MVulkanTexture texture);
    inline std::shared_ptr<Camera> GetCamera()const { return camera; }

    MVulkanSampler GetGlobalSampler()const;

    void CreateBuffer(std::shared_ptr<Buffer> buffer, const void* data, size_t size);

    template<class T>
    void CreateImage(std::shared_ptr<MVulkanTexture> texture, std::vector<MImage<T>*> images, bool calculateMipLevels = false) {
        MImage<T>* image = images[0];
        
        uint32_t mipLevels = 1;
        if (calculateMipLevels) {
            mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(image->Width(), image->Height())))) + 1;
        }

        ImageCreateInfo imageInfo{};
        imageInfo.width = image->Width();
        imageInfo.height = image->Height();
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.format = image->Format();
        imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.mipLevels = mipLevels;

        ImageViewCreateInfo viewInfo{};
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.format = image->Format();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        //viewInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = mipLevels;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = images.size();
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;

        generalGraphicList.Reset();
        generalGraphicList.Begin();

        texture->CreateAndLoadData(&generalGraphicList, device, imageInfo, viewInfo, images, 0);

        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        graphicsQueue.WaitForQueueComplete();
    }

    inline uint32_t GetUniformBufferOffsetAlignment() const {
        return device.GetUniformBufferOffsetAlignment();
    }
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
    void createSkyboxTexture();
    void createIrradianceCubemapTexture();
    void preComputeIrradianceCubemap();
    void createSampler();
    //void loadModel();
    void loadScene();
    void createLight();
    
    void createSyncObjects();
    //void recordGbufferCommandBuffer(uint32_t imageIndex);
    //void recordShadowCommandBuffer(uint32_t imageIndex);
    //void recordFinalCommandBuffer(uint32_t imageIndex);
    void recordCommandBuffer(
        uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount);

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

    void createTempTexture();
    //std::vector<VkDescriptorBufferInfo> generateDescriptorBufferInfos(std::vector<VkBuffer> buffers, std::vector<ShaderResourceInfo> resourceInfos);
private:
    uint32_t m_frameId = 0;

    std::shared_ptr<RenderPass> irradianceConvolutionPass;

    std::shared_ptr<RenderPass> gbufferPass;
    std::shared_ptr<RenderPass> shadowPass;
    std::shared_ptr<RenderPass> lightingPass;
    std::shared_ptr<RenderPass> skyboxPass;

    uint16_t windowWidth = 800, windowHeight = 600;
    //uint32_t m_frameId = 0;

    MWindow* window = nullptr;

    bool enableValidationLayer;
    
    MVulkanInstance instance;
    MVulkanDevice device;
    MVulkanSwapchain swapChain;

    MVulkanDescriptorSetAllocator allocator;

    VkSurfaceKHR surface;
    MVulkanCommandQueue graphicsQueue;
    MVulkanCommandQueue presentQueue;
    MVulkanCommandQueue transferQueue;
    MVulkanCommandAllocator commandAllocator;

    std::vector<MGraphicsCommandList> graphicsLists;
    MGraphicsCommandList generalGraphicList;
    MGraphicsCommandList presentList;
    MGraphicsCommandList transferList;

    std::shared_ptr<Buffer> squadVertexBuffer;
    std::shared_ptr<Buffer> squadIndexBuffer;

    MVulkanSampler sampler;
    MVulkanTexture testTexture;
    MImage<unsigned char> image;

    MVulkanTexture skyboxTexture;
    MVulkanTexture irradianceTexture;

    std::vector<MVulkanSemaphore> imageAvailableSemaphores;
    std::vector<MVulkanSemaphore> finalRenderFinishedSemaphores;
    std::vector<MVulkanFence> inFlightFences;
    uint32_t currentFrame = 0;

    std::shared_ptr<Camera> camera;

    std::shared_ptr<Camera> directionalLightCamera;

    std::shared_ptr<Scene> scene;
    std::shared_ptr<Scene> cube;
    std::shared_ptr<Light> directionalLight;

    std::vector<MVulkanSampler> globalSamplers;

    std::shared_ptr<Buffer> lightPassIndirectBuffer;
};

#endif