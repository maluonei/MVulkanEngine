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
    MVulkanDescriptorSetAllocator allocator, std::vector<std::vector<VkImageView>> imageViews)
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

void RenderPass::UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> imageViews)
{
    MVulkanDescriptorSetWrite write;
    std::vector < std::vector<VkDescriptorImageInfo>> imageInfos(m_info.frambufferCount);
    for (int i = 0; i < m_info.frambufferCount; i++) {
        imageInfos[i].resize(imageViews.size());
        for (auto j = 0; j < imageViews.size(); j++) {
            imageInfos[i][j].sampler = Singleton<MVulkanEngine>::instance().GetGlobalSampler().GetSampler();
            imageInfos[i][j].imageView = imageViews[i][j];
            imageInfos[i][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(m_info.frambufferCount);
    for (int i = 0; i < m_info.frambufferCount; i++) {
        bufferInfos[i].resize(m_cbvCount);

        for (auto binding = 0; binding < m_cbvCount; binding++) {
            bufferInfos[i][binding].buffer = m_uniformBuffers[binding][i].GetBuffer();
            bufferInfos[i][binding].offset = 0;
            bufferInfos[i][binding].range = m_shader->GetBufferSizeBinding(binding);
        }
    }

    write.Update(m_device.GetDevice(), m_descriptorSet.Get(), bufferInfos, imageInfos, m_info.frambufferCount);
}

void RenderPass::CreatePipeline(MVulkanDescriptorSetAllocator allocator, std::vector<std::vector<VkImageView>> imageViews )
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
    m_descriptorSet.Create(m_device.GetDevice(), allocator.Get(), m_descriptorLayouts.Get(), m_info.frambufferCount);

    //for (auto i = 0; i < bindings.size(); i++) {
    //    auto binding = bindings[i];
    //    switch (binding.binding.descriptorType) {
    //        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    //            std::vector<MCBV> buffers(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    //            for (auto i = 0; i < Singleton<GlobalConfig>::instance().GetMaxFramesInFlight(); i++) {
    //                BufferCreateInfo bufferCreateInfo{};
    //                bufferCreateInfo.size = binding.size;
    //                buffers[i].Create(m_device, bufferCreateInfo);
    //            }
    //
    //            m_uniformBuffers.insert(std::make_pair(binding.binding.binding, buffers));
    //    }
    //}

    uint32_t cbvCount = 0;
    uint32_t samplerCount = 0;
    for (auto i = 0; i < bindings.size(); i++) {
        switch (bindings[i].binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            for (auto j = 0; j < m_info.frambufferCount; j++) {
                MCBV buffer;
                BufferCreateInfo info;
                info.size = bindings[i].size;
                buffer.Create(m_device, info);
                m_uniformBuffers[i].push_back(buffer);
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

    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(m_info.frambufferCount);
    std::vector<std::vector<VkDescriptorImageInfo>> imageInfos(m_info.frambufferCount);

    for (auto i = 0; i < m_info.frambufferCount; i++) {
        bufferInfos[i].resize(cbvCount);
    
        for (auto binding = 0; binding < cbvCount; binding++) {
            bufferInfos[i][binding].buffer = m_uniformBuffers[binding][i].GetBuffer();
            bufferInfos[i][binding].offset = 0;
            bufferInfos[i][binding].range = m_shader->GetBufferSizeBinding(binding);
        }
    }

    for (auto i = 0; i < m_info.frambufferCount; i++) {
        imageInfos[i].resize(samplerCount);

        for (auto binding = 0; binding < samplerCount; binding++) {
            imageInfos[i][binding].sampler = Singleton<MVulkanEngine>::instance().GetGlobalSampler().GetSampler();
            imageInfos[i][binding].imageView = imageViews[i][binding];
            imageInfos[i][binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    m_cbvCount = cbvCount;
    m_textureCount = samplerCount;

    write.Update(m_device.GetDevice(), m_descriptorSet.Get(), bufferInfos, imageInfos, m_info.frambufferCount);

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
    gbufferFormats.initialLayout = m_info.initialLayout;
    gbufferFormats.finalLayout = m_info.finalLayout;

    m_renderPass.Create(m_device, gbufferFormats);
}

void RenderPass::CreateFrameBuffers(
    MVulkanSwapchain swapChain, MVulkanCommandQueue commandQueue, MGraphicsCommandList commandList)
{
    commandList.Reset();
    commandList.Begin();

    m_frameBuffers.resize(m_info.frambufferCount);

    std::vector<VkImageView> imageViews = swapChain.GetSwapChainImageViews();

    for (auto i = 0; i < m_info.frambufferCount; i++) {
        FrameBufferCreateInfo info{};

        info.useSwapchainImageViews = m_info.useSwapchainImages;
        info.useAttachmentResolve = m_info.useAttachmentResolve;
        info.renderPass = m_renderPass.Get();
        info.extent = m_info.extent;
        info.imageAttachmentFormats = m_info.imageAttachmentFormats.data();
        info.numAttachments = static_cast<uint32_t>(m_info.imageAttachmentFormats.size());
        info.colorAttachmentResolvedViews = m_info.colorAttachmentResolvedViews;
        info.useAttachmentResolve = m_info.useAttachmentResolve;
        info.swapchainImageViews = &imageViews[i];

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

void RenderPass::TransitionFrameBufferImageLayout(MVulkanCommandQueue queue, MGraphicsCommandList commandList, VkImageLayout oldLayout, VkImageLayout newLayout) {
    commandList.Reset();
    commandList.Begin();

    MVulkanImageMemoryBarrier barrier{};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    std::vector<MVulkanImageMemoryBarrier> barriers;

    for (auto i = 0; i < m_frameBuffers.size(); i++) {
        for (auto j = 0; j < m_frameBuffers[i].ColorAttachmentsCount(); j++) {
            barrier.image = m_frameBuffers[i].GetImage(j);
            barriers.push_back(barrier);
        }
    }

    commandList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    commandList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandList.GetBuffer();

    queue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    queue.WaitForQueueComplete();
}

void RenderPass::LoadCBV() {
    for (auto binding = 0; binding < m_cbvCount; binding++) {
        for (auto j = 0; j < m_info.frambufferCount; j++) {
            m_uniformBuffers[binding][j].UpdateData(m_device, m_shader->GetData(binding));
        }
    }
}