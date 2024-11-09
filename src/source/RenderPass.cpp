#include "RenderPass.hpp"
#include "MVulkanRHI/MVulkanShader.hpp"
#include "Shaders/ShaderModule.hpp"
#include "Managers/GlobalConfig.hpp"

RenderPass::RenderPass(MVulkanDevice device, RenderPassCreateInfo info):
    m_device(device), m_info(info)
{

}


void RenderPass::Create()
{
}

void RenderPass::CreatePipeline(MVulkanDescriptorSetAllocator allocator)
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

    //GbufferShader::UniformBufferObject ubo{};
    //ubo.Model = glm::mat4(1.f);
    //ubo.View = camera->GetViewMatrix();
    //ubo.Projection = camera->GetProjMatrix();
    //gbufferShader->SetUBO(0, &ubo);
    //
    //gbufferCbvs0.resize(MAX_FRAMES_IN_FLIGHT);
    //BufferCreateInfo _infoUBO0{};
    //_infoUBO0.size = gbufferShader->GetBufferSizeBinding(0);
    //
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    gbufferCbvs0[i].Create(device, _infoUBO0);
    //}

    MVulkanDescriptorSetWrite write;

    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(Singleton<GlobalConfig>::instance().GetMaxFramesInFlight());
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //    bufferInfos[i].resize(1);
    //
    //    bufferInfos[i][0].buffer = gbufferCbvs0[i].GetBuffer();
    //    bufferInfos[i][0].offset = 0;
    //    bufferInfos[i][0].range = phongShader.GetBufferSizeBinding(0);
    //}

    std::vector<std::vector<VkDescriptorImageInfo>> imageInfos(0);

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