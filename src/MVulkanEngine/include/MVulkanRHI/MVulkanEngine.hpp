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
#include "MVulkanDescriptorWrite.hpp"
#include "UIRenderer.hpp"
//#include "Camera.hpp"

#define MAX_FRAMES_IN_FLIGHT 2

class MWindow;
class Camera;
class Scene;
class RenderPass;
class DynamicRenderPass;
class ComputePass;
class Light;

enum class MQueueType {
    GRAPHICS,
    TRANSFER,
    PRESENT,
    COMPUTE,
    RAYTRACING,
};

class MVulkanEngine: public Singleton<MVulkanEngine> {
//friend RenderApplication;
public:
    MVulkanEngine();
    ~MVulkanEngine();
    void Clean();

    void Init();

    void SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight);

    inline MVulkanDevice GetDevice()const { return m_device; }

    void GenerateMipMap(MVulkanTexture texture);
    inline std::shared_ptr<Camera> GetCamera()const { return m_camera; }

    void CreateBuffer(std::shared_ptr<Buffer> buffer, const void* data, size_t size);

    void CreateColorAttachmentImage(
        std::shared_ptr<MVulkanTexture> texture,
        VkExtent2D extent,
        VkFormat format
    );

    void CreateDepthAttachmentImage(
        std::shared_ptr<MVulkanTexture> texture,
        VkExtent2D extent,
        VkFormat format
    );

    void CreateImage(std::shared_ptr<MVulkanTexture> texture, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo, VkImageLayout layout);
    void CreateImage(std::shared_ptr<MVulkanTexture> texture, ImageCreateInfo imageInfo, ImageViewCreateInfo viewInfo);

    void LoadTextureData(std::shared_ptr<MVulkanTexture> texture, void* data, size_t size, uint32_t offset);
    void LoadTextureData(std::shared_ptr<MVulkanTexture> texture, void* data, size_t size, uint32_t offset,
        VkOffset3D origin, VkExtent3D extent);

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

        m_generalGraphicList.Reset();
        m_generalGraphicList.Begin();

        texture->CreateAndLoadData(&m_generalGraphicList, m_device, imageInfo, viewInfo, images, 0);

        m_generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_generalGraphicList.GetBuffer();

        m_graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        m_graphicsQueue.WaitForQueueComplete();

        //auto vkCmdBeginDebugUtilsLabelEXT =
        //    (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
        //
        //auto vkCmdEndDebugUtilsLabelEXT =
        //    (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
    }

    inline uint32_t GetUniformBufferOffsetAlignment() const {
        return m_device.GetUniformBufferOffsetAlignment();
    }

    MVulkanCommandQueue& GetCommandQueue(MQueueType type);
    MGraphicsCommandList& GetCommandList(MQueueType type);
    inline MComputeCommandList& GetComputeCommandList() { return m_computeList; }
    inline MRaytracingCommandList& GetRaytracingCommandList(){return m_raytracingList;}
    MGraphicsCommandList& GetGraphicsList(int i);

    //void CreateRenderPass(std::shared_ptr<RenderPass> renderPass, 
    //    std::shared_ptr<ShaderModule> shader, std::vector<std::vector<VkImageView>> imageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures = {});
    //
    //void CreateRenderPass(std::shared_ptr<RenderPass> renderPass,
    //    std::shared_ptr<ShaderModule> shader, 
    //    std::vector<uint32_t> storageBufferSizes, 
    //    std::vector<std::vector<VkImageView>> imageViews, 
    //    std::vector<std::vector<VkImageView>> storageImageViews,
    //    std::vector<VkSampler> samplers, 
    //    std::vector<VkAccelerationStructureKHR> accelerationStructures = {});
    //
    //void CreateRenderPass(std::shared_ptr<RenderPass> renderPass,
    //    std::shared_ptr<ShaderModule> shader,
    //    std::vector<StorageBuffer> storageBuffers,
    //    std::vector<std::vector<VkImageView>> imageViews,
    //    std::vector<std::vector<VkImageView>> storageImageViews,
    //    std::vector<VkSampler> samplers,
    //    std::vector<VkAccelerationStructureKHR> accelerationStructures = {});
    //
    //void CreateRenderPass(std::shared_ptr<RenderPass> renderPass,
    //    std::shared_ptr<MeshShaderModule> shader,
    //    std::vector<StorageBuffer> storageBuffers,
    //    std::vector<std::vector<VkImageView>> imageViews,
    //    std::vector<std::vector<VkImageView>> storageImageViews,
    //    std::vector<VkSampler> samplers,
    //    std::vector<VkAccelerationStructureKHR> accelerationStructures = {});

    //void CreateRenderPass(std::shared_ptr<RenderPass> renderPass,
    //    std::shared_ptr<ShaderModule> shader, std::vector<PassResources> resources
    //);

    void CreateRenderPass(std::shared_ptr<RenderPass> renderPass,
        std::shared_ptr<ShaderModule> shader
    );

    //void CreateDynamicRenderPass(std::shared_ptr<DynamicRenderPass> renderPass,
    //    std::shared_ptr<ShaderModule> shader,
    //    std::vector<StorageBuffer> storageBuffers,
    //    std::vector<std::vector<VkImageView>> imageViews,
    //    std::vector<std::vector<VkImageView>> storageImageViews,
    //    std::vector<VkSampler> samplers,
    //    std::vector<VkAccelerationStructureKHR> accelerationStructures = {});

    void CreateComputePass(std::shared_ptr<ComputePass> computePass, std::shared_ptr<ComputeShaderModule> shader);

    //void CreateComputePass(std::shared_ptr<ComputePass> computePass, std::shared_ptr<ComputeShaderModule> shader,
    //    std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos,
    //    std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures = {});
    //
    //void CreateComputePass(std::shared_ptr<ComputePass> computePass, 
    //    std::shared_ptr<ComputeShaderModule> shader,
    //    std::vector<uint32_t> storageBufferSizes, 
    //    std::vector<std::vector<VkImageView>> seperateImageViews, 
    //    std::vector<std::vector<VkImageView>> storageImageViews,
    //    std::vector<VkSampler> samplers, 
    //    std::vector<VkAccelerationStructureKHR> accelerationStructures = {});
    //
    //void CreateComputePass(std::shared_ptr<ComputePass> computePass,
    //    std::shared_ptr<ComputeShaderModule> shader,
    //    std::vector<StorageBuffer> storageBuffers,
    //    std::vector<std::vector<VkImageView>> seperateImageViews,
    //    std::vector<std::vector<VkImageView>> storageImageViews,
    //    std::vector<VkSampler> samplers,
    //    std::vector<VkAccelerationStructureKHR> accelerationStructures = {});


    //void CreateComputePass(std::shared_ptr<ComputePass> computePass, std::shared_ptr<ComputeShaderModule> shader,
    //    std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<VkImageView>> storageImageViews,
    //    std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures = {});
    
    inline VkExtent2D GetSwapchainImageExtent()const { return m_swapChain.GetSwapChainExtent(); }
    inline VkFormat GetSwapchainImageFormat()const { return m_swapChain.GetSwapChainImageFormat(); }
    inline uint32_t GetSwapchainImageCount()const { return m_swapChain.GetImageCount(); }

    bool WindowShouldClose() const;
    void PollWindowEvents();

    VkResult AcquireNextSwapchainImage(uint32_t& imageIndex, uint32_t currentFrame);

    inline MVulkanFence GetInFlightFence(uint32_t index) const { return m_inFlightFences[index]; }
    inline MVulkanSemaphore GetImageAvilableSemaphore(uint32_t index)const { return m_imageAvailableSemaphores[index]; }
    inline MVulkanSemaphore GetFinalRenderFinishedSemaphoresSemaphore(uint32_t index)const { return m_finalRenderFinishedSemaphores[index]; }

    void SubmitGraphicsCommands(uint32_t imageIndex, uint32_t currentFrame);
    //void SubmitComputeCommands(uint32_t imageIndex, uint32_t currentFrame);
    //void SubmitCommandsAndPresent(uint32_t imageIndex, uint32_t currentFrame, std::function<void()> recreateSwapchain);

    void RecordCommandBuffer(
        uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount,
        bool flipY = true);

    void RecordCommandBuffer(
        uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount,
        std::string eventName, bool flipY = true);

    void RecordCommandBuffer(
        uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<StorageBuffer> indirectBuffer, uint32_t indirectBufferOffset, uint32_t indirectCount,
        bool flipY = true);

    void RecordCommandBuffer(
        uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<StorageBuffer> indirectBuffer, uint32_t indirectBufferOffset, uint32_t indirectCount,
        std::string eventName, bool flipY = true);

    void RecordCommandBuffer(
        uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, 
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount,
        bool flipY = true);

    void RecordCommandBuffer(
        uint32_t frameIndex,
        std::shared_ptr<RenderPass> renderPass,
        uint32_t currentFrame,
        RenderingInfo& renderingInfo,
        std::shared_ptr<Buffer> vertexBuffer,
        std::shared_ptr<Buffer> indexBuffer,
        std::shared_ptr<Buffer> indirectBuffer,
        uint32_t indirectCount,
        std::string eventName, bool flipY = true);

    void RecordCommandBuffer2(
        uint32_t frameIndex, 
        std::shared_ptr<DynamicRenderPass> renderPass,
        uint32_t currentFrame,
        const RenderingInfo& renderingInfo,
        std::shared_ptr<Buffer> vertexBuffer, 
        std::shared_ptr<Buffer> indexBuffer,
        std::shared_ptr<Buffer> indirectBuffer, 
        uint32_t indirectCount,
        std::string eventName, bool flipY = true);


    void RecordComputeCommandBuffer(std::shared_ptr<ComputePass> computePass,
        uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    void RecordComputeCommandBuffer(std::shared_ptr<ComputePass> computePass,
        uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, std::string eventName);

    void RecordMeshShaderCommandBuffer(
        uint32_t frameIndex, std::shared_ptr<RenderPass> renderPass, uint32_t currentFrame,
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ,
        std::string eventName = "",
        bool flipY = true);

    void SetCamera(std::shared_ptr<Camera> camera);

    bool RecreateSwapchain();

    void RecreateRenderPassFrameBuffer(std::shared_ptr<RenderPass> renderPass);

    std::shared_ptr<MVulkanTexture> GetPlaceHolderTexture();
    std::shared_ptr<StorageBuffer> CreateStorageBuffer(BufferCreateInfo info, void* data = nullptr);

    void TransitionImageLayout(std::vector<MVulkanImageMemoryBarrier> barriers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
    void TransitionImageLayout(MVulkanImageMemoryBarrier barrier, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);

    void TransitionImageLayout2(int commandListId, std::vector<MVulkanImageMemoryBarrier> barriers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
    void TransitionImageLayout2(MGraphicsCommandList list, std::vector<MVulkanImageMemoryBarrier> barriers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);

    void RenderUI(uint32_t imageIndex, uint32_t currentFrame);

    void Present(
        uint32_t currentFrame, const uint32_t* imageIndex, std::function<void()> recreateSwapchain);

    const MVulkanSwapchain GetSwapchain() { return m_swapChain; }
private: 
    void initVulkan();
    void initUIRenderer();

    void createInstance();
    void createDevice();
    void createSurface();
    void createSwapChain();

    void createUIRenderPass();

    void transitionSwapchainImageFormat();
    void transitionFramebufferImageLayout();

    void createDescriptorSetAllocator();

    void createCommandQueue();
    void createCommandAllocator();
    void createCommandList();

    void createSyncObjects();

    void recordCommandBuffer(
        uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount,
        bool flipY = true);

    void recordCommandBuffer(
        uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<Buffer> indirectBuffer, uint32_t indirectCount,
        std::string eventName, bool flipY = true);

    void recordCommandBuffer(
        uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<StorageBuffer> indirectBuffer, uint32_t indirectBufferOffset, uint32_t indirectCount,
        bool flipY = true);

    void recordCommandBuffer(
        uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList,
        std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer, std::shared_ptr<StorageBuffer> indirectBuffer, uint32_t indirectBufferOffset, uint32_t indirectCount,
        std::string eventName, bool flipY = true);

    void recordCommandBuffer(
        uint32_t imageIndex,
        std::shared_ptr<RenderPass> renderPass,
        //MGraphicsCommandList commandList,
        uint32_t currentFrame,
        RenderingInfo& renderingInfo,
        std::shared_ptr<Buffer> vertexBuffer,
        std::shared_ptr<Buffer> indexBuffer,
        std::shared_ptr<Buffer> indirectBuffer,
        uint32_t indirectCount,
        std::string eventName,
        bool flipY = true);

    void prepareRenderingInfo(uint32_t imageIndex, RenderingInfo& renderingInfo, uint32_t currentFrame);
    void swapchainImage2Present(uint32_t imageIndex, uint32_t currentFrame);
    
    void recordCommandBuffer2(
        uint32_t imageIndex, 
        std::shared_ptr<DynamicRenderPass> renderPass, 
        MGraphicsCommandList commandList,
        const RenderingInfo& renderingInfo,
        std::shared_ptr<Buffer> vertexBuffer, 
        std::shared_ptr<Buffer> indexBuffer, 
        std::shared_ptr<Buffer> indirectBuffer,
        uint32_t indirectCount,
        std::string eventName, 
        bool flipY = true);


    void recordComputeCommandBuffer(std::shared_ptr<ComputePass> computePass,
        uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    void recordComputeCommandBuffer(std::shared_ptr<ComputePass> computePass,
        uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, std::string eventName);

    void recordMeshShaderCommandBuffer(
        uint32_t imageIndex, std::shared_ptr<RenderPass> renderPass, MGraphicsCommandList commandList,
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ,
        std::string eventName = "", bool flipY = true);

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

    //void present(
    //    VkSwapchainKHR* swapChains, uint32_t currentFrame,
    //    const uint32_t* imageIndex, std::function<void()> recreateSwapchain);

    //void present(
    //    VkSwapchainKHR* swapChains, VkSemaphore* waitSemaphore, 
    //    const uint32_t* imageIndex, std::function<void()> recreateSwapchain);

    void createPlaceHolderTexture();

protected:
    virtual void InitSingleton();
private:
    uint16_t m_windowWidth = 800, m_windowHeight = 600;

    MWindow*                m_window = nullptr;
    bool                    m_enableValidationLayer;
    MVulkanInstance         m_instance;
    MVulkanDevice           m_device;
    MVulkanSwapchain        m_swapChain;

    MVulkanDescriptorSetAllocator m_allocator;

    VkSurfaceKHR            m_surface;
    MVulkanCommandQueue     m_graphicsQueue;
    MVulkanCommandQueue     m_presentQueue;
    MVulkanCommandQueue     m_transferQueue;
    MVulkanCommandQueue     m_computeQueue;
    MVulkanCommandAllocator m_commandAllocator;

    std::vector<MGraphicsCommandList> m_graphicsLists;
    MGraphicsCommandList    m_generalGraphicList;
    MGraphicsCommandList    m_presentList;
    MGraphicsCommandList    m_transferList;
    MComputeCommandList     m_computeList;
    MRaytracingCommandList  m_raytracingList;

    std::shared_ptr<MVulkanRenderPass> m_uiRenderPass = nullptr;
    UIRenderer              m_uiRenderer;

    std::vector<MVulkanSemaphore> m_imageAvailableSemaphores;
    std::vector<MVulkanSemaphore> m_finalRenderFinishedSemaphores;
    std::vector<MVulkanSemaphore> m_uiRenderFinishedSemaphores;
    std::vector<MVulkanFence> m_inFlightFences;
    uint32_t currentFrame = 0;

    std::shared_ptr<Camera> m_camera = nullptr;

    std::shared_ptr<MVulkanTexture> m_placeHolderTexture;
};


#endif