#include "RenderPass.hpp"
#include "MVulkanRHI/MVulkanShader.hpp"
#include "Shaders/ShaderModule.hpp"
#include "Managers/GlobalConfig.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

RenderPass::RenderPass(MVulkanDevice device, RenderPassCreateInfo info):
    m_device(device), m_info(info)
{

}

void RenderPass::Create(std::shared_ptr<ShaderModule> shader,
    MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList,
    MVulkanDescriptorSetAllocator allocator, std::vector<VkImageView> imageViews)
{
    SetShader(shader);
    CreateRenderPass();
    CreateFrameBuffers(swapChain, commandQueue, commandList);
    CreatePipeline(allocator, imageViews);
}

void RenderPass::Clean()
{
    m_pipeline.Clean(m_device.GetDevice());

    for (auto frameBuffer : m_frameBuffers) {
        frameBuffer.Clean(m_device.GetDevice());
    }

    m_renderPass.Clean(m_device.GetDevice());
}

void RenderPass::RecreateFrameBuffers(MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList, VkExtent2D extent) {
    m_info.extent = extent;

    for (auto frameBuffer : m_frameBuffers) {
        frameBuffer.Clean(m_device.GetDevice());
    }

    CreateFrameBuffers(swapChain, commandQueue, commandList);
}

void RenderPass::CreatePipeline(MVulkanDescriptorSetAllocator allocator, std::vector<VkImageView> imageViews )
{
    MVulkanShaderReflector vertReflector(m_shader->GetVertexShader().GetShader());
    MVulkanShaderReflector fragReflector(m_shader->GetFragmentShader().GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();
    //fragReflector.GenerateDescriptorSet();

    ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());

    m_descriptorLayouts.Create(m_device.GetDevice(), bindings);
    m_descriptorSet.Create(m_device.GetDevice(), allocator.Get(), m_descriptorLayouts.Get(), Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());

    for (auto i = 0; i < bindings.size(); i++) {
        auto binding = bindings[i];
        switch (binding.binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                std::vector<MCBV> buffers(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
                for (auto i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++) {
                    BufferCreateInfo bufferCreateInfo{};
                    bufferCreateInfo.size = binding.size;
                    buffers[i].Create(m_device, bufferCreateInfo);
                }

                m_uniformBuffers.insert(std::make_pair(binding.binding.binding, buffers));
        }
    }

    uint32_t cbvCount = 0;
    uint32_t samplerCount = 0;
    for (auto i = 0; i < bindings.size(); i++) {
        switch (bindings[i].binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            for (auto j = 0; j < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); j++) {
                MCBV buffer;
                BufferCreateInfo info;
                info.size = bindings[i].size;
                buffer.Create(m_device, info);
                m_uniformBuffers[cbvCount].push_back(buffer);
            }
            cbvCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        {
            samplerCount++;
            break;
        }
        }
    }

    MVulkanDescriptorSetWrite write;

    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    std::vector<std::vector<VkDescriptorImageInfo>> imageInfos(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());

    for (auto i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++) {
        bufferInfos[i].resize(cbvCount);
    
        for (auto binding = 0; binding < cbvCount; binding++) {
            bufferInfos[i][binding].buffer = m_uniformBuffers[i][binding].GetBuffer();
            bufferInfos[i][binding].offset = 0;
            bufferInfos[i][binding].range = m_shader->GetBufferSizeBinding(binding);
        }
    }

    for (auto i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++) {
        imageInfos[i].resize(samplerCount);

        for (auto binding = 0; binding < samplerCount; binding++) {
            imageInfos[i][binding].sampler = Singleton<MVulkanEngine>::instance().GetGlobalSampler().GetSampler();
            imageInfos[i][binding].imageView = imageViews[binding];
            imageInfos[i][binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    write.Update(m_device.GetDevice(), m_descriptorSet.Get(), bufferInfos, imageInfos, Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());

    m_pipeline.Create(m_device, 
        m_shader->GetVertexShader().GetShaderModule(), m_shader->GetFragmentShader().GetShaderModule(),
        m_renderPass.Get(), info, m_descriptorLayouts.Get(), m_info.imageAttachmentFormats.size());

    m_shader->Clean(m_device.GetDevice());
}

void RenderPass::CreateRenderPass()
{
    RenderPassFormatsInfo gbufferFormats{};
    gbufferFormats.useResolvedRef = m_info.useAttachmentResolve;
    gbufferFormats.imageFormats = m_info.imageAttachmentFormats;
    gbufferFormats.depthFormat = m_device.FindDepthFormat();
    
    m_renderPass.Create(m_device, gbufferFormats);
}

void RenderPass::CreateFrameBuffers(
    MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList)
{
    m_frameBuffers.resize(swapChain.GetImageCount());

    commandList.Reset();
    commandList.Begin();

    m_frameBuffers.resize(m_info.imageAttachmentFormats.size());
    for (auto i = 0; i < m_info.imageAttachmentFormats.size(); i++) {
        FrameBufferCreateInfo info{};

        info.finalFrameBuffer = m_info.useSwapchainImages;
        info.useAttachmentResolve = m_info.useAttachmentResolve;
        info.renderPass = m_renderPass.Get();
        info.extent = m_info.extent;
        info.imageAttachmentFormats = m_info.imageAttachmentFormats.data();
        info.numAttachments = static_cast<uint32_t>(m_info.imageAttachmentFormats.size());
        info.colorAttachmentResolvedViews = m_info.colorAttachmentResolvedViews;
        info.useAttachmentResolve = m_info.useAttachmentResolve;

        m_frameBuffers[i].Create(m_device, info);
    }

    commandList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandList.GetBuffer();

    commandQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    commandQueue.WaitForQueueComplete();
}

void RenderPass::SetShader(std::shared_ptr<ShaderModule> shader)
{
    m_shader = shader;
    m_shader->Create(m_device.GetDevice());
}

void RenderPass::SetUBO(uint8_t index, void* data) {
    m_shader->SetUBO(index, data);
}