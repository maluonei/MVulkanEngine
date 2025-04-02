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
    MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
    MVulkanDescriptorSetAllocator& allocator, 
    std::vector<std::vector<VkImageView>> imageViews, std::vector<VkSampler> samplers, 
    std::vector<VkAccelerationStructureKHR> accelerationStructure)
{
    SetShader(shader);
    CreateRenderPass();
    CreateFrameBuffers(swapChain, commandQueue, commandList);
    CreatePipeline(allocator, imageViews, samplers, accelerationStructure);
}

void RenderPass::Create(std::shared_ptr<ShaderModule> shader,
    MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
    MVulkanDescriptorSetAllocator& allocator, std::vector<uint32_t> storageBufferSizes,
    std::vector<std::vector<VkImageView>> imageViews, std::vector<std::vector<VkImageView>> storageImageViews, std::vector<VkSampler> samplers,
    std::vector<VkAccelerationStructureKHR> accelerationStructure)
{
    SetShader(shader);
    CreateRenderPass();
    CreateFrameBuffers(swapChain, commandQueue, commandList);
    CreatePipeline(allocator, storageBufferSizes, imageViews, storageImageViews, samplers, accelerationStructure);
}

void RenderPass::Create(std::shared_ptr<ShaderModule> shader,
    MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, 
    MGraphicsCommandList& commandList,
    MVulkanDescriptorSetAllocator& allocator, 
    std::vector<StorageBuffer> storageBuffers,
    std::vector<std::vector<VkImageView>> imageViews, 
    std::vector<std::vector<VkImageView>> storageImageViews, 
    std::vector<VkSampler> samplers,
    std::vector<VkAccelerationStructureKHR> accelerationStructure)
{
    SetShader(shader);
    CreateRenderPass();
    CreateFrameBuffers(swapChain, commandQueue, commandList);
    CreatePipeline(allocator, storageBuffers, imageViews, storageImageViews, samplers, accelerationStructure);
}

void RenderPass::Create(std::shared_ptr<MeshShaderModule> meshShader,
    MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList,
    MVulkanDescriptorSetAllocator& allocator,
    std::vector<StorageBuffer> storageBuffers,
    std::vector<std::vector<VkImageView>> imageViews,
    std::vector<std::vector<VkImageView>> storageImageViews,
    std::vector<VkSampler> samplers,
    std::vector<VkAccelerationStructureKHR> accelerationStructure) {
    SetMeshShader(meshShader);
    CreateRenderPass();
    CreateFrameBuffers(swapChain, commandQueue, commandList);
    CreateMeshShaderPipeline(allocator, storageBuffers, imageViews, storageImageViews, samplers, accelerationStructure);
}

void RenderPass::Clean()
{
    for (auto i = 0; i < m_uniformBuffers.size(); i++) {
        for (auto j = 0; j < m_uniformBuffers[i].size(); j++) {
            m_uniformBuffers[i][j].Clean();
        }
        m_uniformBuffers[i].clear();
    }
    m_uniformBuffers.clear();

    for (auto i = 0; i < m_storageBuffer.size(); i++) {
        m_storageBuffer[i].Clean();
    }
    m_uniformBuffers.clear();

    m_descriptorLayouts.Clean();

    for (auto descriptorSet : m_descriptorSets) {
        descriptorSet.Clean();
    }

    m_pipeline.Clean();

    for (auto frameBuffer : m_frameBuffers) {
        frameBuffer.Clean();
    }

    m_renderPass.Clean();
}

void RenderPass::RecreateFrameBuffers(MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList, VkExtent2D extent) {
    m_info.extent = extent;

    for (auto frameBuffer : m_frameBuffers) {
        frameBuffer.Clean();
    }
    m_frameBuffers.clear();

    CreateFrameBuffers(swapChain, commandQueue, commandList);
}

void RenderPass::UpdateDescriptorSetWrite(
    std::vector<std::vector<VkImageView>> imageViews, std::vector<VkSampler> samplers, 
    std::vector<VkAccelerationStructureKHR> accelerationStructure)
{
    for (auto i = 0; i < m_frameBuffers.size(); i++) {
        MVulkanDescriptorSetWrite write;

        std::vector<std::vector<VkDescriptorImageInfo>> combinedImageInfos(m_combinedImageCount);
        std::vector<std::vector<VkDescriptorImageInfo>> separateImageInfos(m_separateImageCount);
        std::vector<std::vector<VkDescriptorImageInfo>> storageImageInfos(0);
        std::vector<VkDescriptorImageInfo> separateSamplerInfos(m_separateSamplerCount);

        for (auto binding = 0; binding < m_combinedImageCount; binding++) {
            combinedImageInfos[binding].resize(imageViews[binding].size());
            for (auto j = 0; j < imageViews[binding].size(); j++) {
                combinedImageInfos[binding][j].sampler = samplers[binding];
                combinedImageInfos[binding][j].imageView = imageViews[binding][j];
                combinedImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        for (auto binding = 0; binding < m_separateImageCount; binding++) {
            separateImageInfos[binding].resize(imageViews[binding].size());
            for (auto j = 0; j < imageViews[binding].size(); j++) {
                separateImageInfos[binding][j].sampler = nullptr;
                separateImageInfos[binding][j].imageView = imageViews[binding][j];
                separateImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        for (auto binding = 0; binding < m_separateSamplerCount; binding++) {
            separateSamplerInfos[binding].sampler = samplers[binding];
            separateSamplerInfos[binding].imageView = nullptr;
            separateSamplerInfos[binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        std::vector<std::vector<VkDescriptorBufferInfo>> constantBufferInfos(m_cbvCount);
        std::vector<std::vector<VkDescriptorBufferInfo>> storageBufferInfos(0);

        for (auto binding = 0; binding < m_cbvCount; binding++) {
            constantBufferInfos[binding].resize(m_uniformBuffers[i][binding].GetArrayLength());
            for (auto j = 0; j < m_uniformBuffers[i][binding].GetArrayLength(); j++) {
                constantBufferInfos[binding][j].buffer = m_uniformBuffers[i][binding].GetBuffer();
                constantBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_uniformBuffers[i][binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
                constantBufferInfos[binding][j].range = m_uniformBuffers[i][binding].GetBufferSize();
            }
        }

        write.Update(m_device.GetDevice(), m_descriptorSets[i].Get(), 
            constantBufferInfos, storageBufferInfos, 
            combinedImageInfos, separateImageInfos, storageImageInfos, separateSamplerInfos, accelerationStructure);
    }
}

void RenderPass::UpdateDescriptorSetWrite(
    std::vector<std::vector<VkImageView>> imageViews, std::vector<std::vector<VkImageView>> storageImageViews, std::vector<VkSampler> samplers,
    std::vector<VkAccelerationStructureKHR> accelerationStructure)
{
    for (auto i = 0; i < m_frameBuffers.size(); i++) {
        MVulkanDescriptorSetWrite write;

        std::vector<std::vector<VkDescriptorImageInfo>> combinedImageInfos(m_combinedImageCount);
        std::vector<std::vector<VkDescriptorImageInfo>> separateImageInfos(m_separateImageCount);
        std::vector<std::vector<VkDescriptorImageInfo>> storageImageInfos(m_storageImageCount);
        std::vector<VkDescriptorImageInfo> separateSamplerInfos(m_separateSamplerCount);

        for (auto binding = 0; binding < m_combinedImageCount; binding++) {
            combinedImageInfos[binding].resize(imageViews[binding].size());
            for (auto j = 0; j < imageViews[binding].size(); j++) {
                combinedImageInfos[binding][j].sampler = samplers[binding];
                combinedImageInfos[binding][j].imageView = imageViews[binding][j];
                combinedImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        for (auto binding = 0; binding < m_separateImageCount; binding++) {
            separateImageInfos[binding].resize(imageViews[binding].size());
            for (auto j = 0; j < imageViews[binding].size(); j++) {
                separateImageInfos[binding][j].sampler = nullptr;
                separateImageInfos[binding][j].imageView = imageViews[binding][j];
                separateImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        for (auto binding = 0; binding < m_storageImageCount; binding++) {
            storageImageInfos[binding].resize(storageImageViews[binding].size());
            for (auto j = 0; j < storageImageViews[binding].size(); j++) {
                storageImageInfos[binding][j].sampler = nullptr;
                storageImageInfos[binding][j].imageView = storageImageViews[binding][j];
                storageImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
        }

        for (auto binding = 0; binding < m_separateSamplerCount; binding++) {
            separateSamplerInfos[binding].sampler = samplers[binding];
            separateSamplerInfos[binding].imageView = nullptr;
            separateSamplerInfos[binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        std::vector<std::vector<VkDescriptorBufferInfo>> constantBufferInfos(m_cbvCount);
        std::vector<std::vector<VkDescriptorBufferInfo>> storageBufferInfos(m_storageBufferCount);

        for (auto binding = 0; binding < m_cbvCount; binding++) {
            constantBufferInfos[binding].resize(m_uniformBuffers[i][binding].GetArrayLength());
            for (auto j = 0; j < m_uniformBuffers[i][binding].GetArrayLength(); j++) {
                constantBufferInfos[binding][j].buffer = m_uniformBuffers[i][binding].GetBuffer();
                constantBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_uniformBuffers[i][binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
                constantBufferInfos[binding][j].range = m_uniformBuffers[i][binding].GetBufferSize();
            }
        }

        for (auto binding = 0; binding < m_storageBufferCount; binding++) {
            storageBufferInfos[binding].resize(m_storageBuffer[binding].GetArrayLength());
            for (auto j = 0; j < m_storageBuffer[binding].GetArrayLength(); j++) {
                storageBufferInfos[binding][j].buffer = m_storageBuffer[binding].GetBuffer();
                storageBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_storageBuffer[binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
                storageBufferInfos[binding][j].range = m_storageBuffer[binding].GetBufferSize();
            }
        }

        write.Update(m_device.GetDevice(), m_descriptorSets[i].Get(),
            constantBufferInfos, storageBufferInfos,
            combinedImageInfos, separateImageInfos, storageImageInfos, separateSamplerInfos, accelerationStructure);
    }
}


void RenderPass::CreatePipeline(MVulkanDescriptorSetAllocator& allocator, 
    std::vector<std::vector<VkImageView>> imageViews, std::vector<VkSampler> samplers, 
    std::vector<VkAccelerationStructureKHR> accelerationStructure)
{
    MVulkanShaderReflector vertReflector(m_shader->GetVertexShader().GetShader());
    MVulkanShaderReflector fragReflector(m_shader->GetFragmentShader().GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();

    ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());
    bindings = RemoveRepeatedBindings(bindings);

    m_descriptorLayouts.Create(m_device.GetDevice(), bindings);
    std::vector<VkDescriptorSetLayout> layouts(m_frameBuffers.size(), m_descriptorLayouts.Get());

    m_descriptorSets.resize(layouts.size());
    for (auto i = 0; i < layouts.size(); i++) {
        m_descriptorSets[i].Create(m_device.GetDevice(), allocator.Get(), layouts[i]);
    }

    m_cbvCount = 0;
    m_separateSamplerCount = 0;
    m_separateImageCount = 0;
    m_combinedImageCount = 0;
    m_asCount = 0;

    for (auto binding = 0; binding < bindings.size(); binding++) {
        switch (bindings[binding].binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            m_cbvCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        {
            m_combinedImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            m_separateSamplerCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            m_separateImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            m_storageImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        {
            m_asCount++;
            break;
        }
    }
    }

    m_uniformBuffers.resize(m_info.frambufferCount);

    for (auto i = 0; i < m_info.frambufferCount; i++) {
        m_uniformBuffers[i].resize(m_cbvCount);
        for (auto binding = 0; binding < bindings.size(); binding++) {
            switch (bindings[binding].binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                MCBV buffer;
                BufferCreateInfo info;
                info.arrayLength = bindings[binding].binding.descriptorCount;
                info.size = bindings[binding].size;
                buffer.Create(m_device, info);

                //m_uniformBuffers[i].push_back(buffer);
                m_uniformBuffers[i][bindings[binding].binding.binding] = buffer;
                break;
            }
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            {
                //m_samplerTypes
                break;
            }
            }
        }
    }

    UpdateDescriptorSetWrite(imageViews, samplers, accelerationStructure);

    m_pipeline.Create(m_device.GetDevice(), m_info.pipelineCreateInfo,
        m_shader->GetVertexShader().GetShaderModule(), m_shader->GetFragmentShader().GetShaderModule(),
        m_renderPass.Get(), info, m_descriptorLayouts.Get(), m_info.imageAttachmentFormats.size(),
        m_shader->GetVertEntryPoint(), m_shader->GetFragEntryPoint());

    m_shader->Clean();
}

void RenderPass::CreatePipeline(MVulkanDescriptorSetAllocator& allocator, std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<VkImageView>> imageViews, std::vector<std::vector<VkImageView>> storageImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructure)
{
    MVulkanShaderReflector vertReflector(m_shader->GetVertexShader().GetShader());
    MVulkanShaderReflector fragReflector(m_shader->GetFragmentShader().GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();

    ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());
    //bindings = RemoveRepeatedBindings(bindings);

    if (m_shader->UseGeometryShader()) {
        MVulkanShaderReflector geomReflector(m_shader->GetGeometryShader().GetShader());
        ShaderReflectorOut resourceOutGeom = geomReflector.GenerateShaderReflactorOut();
        std::vector<MVulkanDescriptorSetLayoutBinding> bindingsGeom = resourceOutGeom.GetBindings();
        bindings.insert(bindings.end(), bindingsGeom.begin(), bindingsGeom.end());
    }

    m_descriptorLayouts.Create(m_device.GetDevice(), bindings);
    std::vector<VkDescriptorSetLayout> layouts(m_frameBuffers.size(), m_descriptorLayouts.Get());

    m_descriptorSets.resize(layouts.size());
    for (auto i = 0; i < layouts.size(); i++) {
        m_descriptorSets[i].Create(m_device.GetDevice(), allocator.Get(), layouts[i]);
    }

    m_cbvCount = 0;
    m_separateSamplerCount = 0;
    m_separateImageCount = 0;
    m_combinedImageCount = 0;
    m_asCount = 0;

    for (auto binding = 0; binding < bindings.size(); binding++) {
        switch (bindings[binding].binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            m_cbvCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            m_storageBufferCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        {
            m_combinedImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            m_separateSamplerCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            m_separateImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            m_storageImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        {
            m_asCount++;
            break;
        }
        }
    }

    m_uniformBuffers.resize(m_info.frambufferCount);
    m_storageBuffer.resize(m_storageBufferCount);

    for (auto i = 0; i < m_info.frambufferCount; i++) {
        m_uniformBuffers[i].resize(m_cbvCount);
        for (auto binding = 0; binding < bindings.size(); binding++) {
            switch (bindings[binding].binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                MCBV buffer;
                BufferCreateInfo info;
                info.arrayLength = bindings[binding].binding.descriptorCount;
                info.size = bindings[binding].size;
                buffer.Create(m_device, info);

                //m_uniformBuffers[i].push_back(buffer);
                m_uniformBuffers[i][bindings[binding].binding.binding] = buffer;
                break;
            }
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            {
                StorageBuffer buffer;
                BufferCreateInfo info;
                info.arrayLength = bindings[binding].binding.descriptorCount;
                //info.size = bindings[binding].size;
                info.size = storageBufferSizes[bindings[binding].binding.binding - m_cbvCount];
                buffer.Create(m_device, info);

                m_storageBuffer[bindings[binding].binding.binding - m_cbvCount] = buffer;
                break;
            }
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            {
                //m_samplerTypes
                break;
            }
            }
        }
    }

    UpdateDescriptorSetWrite(imageViews, storageImageViews, samplers, accelerationStructure);

    if (m_shader->UseGeometryShader()) {
        m_pipeline.Create(m_device.GetDevice(), m_info.pipelineCreateInfo,
            m_shader->GetVertexShader().GetShaderModule(), 
            m_shader->GetGeometryShader().GetShaderModule(),
            m_shader->GetFragmentShader().GetShaderModule(),
            m_renderPass.Get(), info, m_descriptorLayouts.Get(), m_info.imageAttachmentFormats.size(),
            m_shader->GetVertEntryPoint(), 
            m_shader->GetGeomEntryPoint(),
            m_shader->GetFragEntryPoint());
    }
    else {
        m_pipeline.Create(m_device.GetDevice(), m_info.pipelineCreateInfo,
            m_shader->GetVertexShader().GetShaderModule(), m_shader->GetFragmentShader().GetShaderModule(),
            m_renderPass.Get(), info, m_descriptorLayouts.Get(), m_info.imageAttachmentFormats.size(),
            m_shader->GetVertEntryPoint(), m_shader->GetFragEntryPoint());
    }

    m_shader->Clean();
}

void RenderPass::CreatePipeline(MVulkanDescriptorSetAllocator& allocator,
    std::vector<StorageBuffer> storageBuffers,
    std::vector<std::vector<VkImageView>> imageViews,
    std::vector<std::vector<VkImageView>> storageImageViews,
    std::vector<VkSampler> samplers,
    std::vector<VkAccelerationStructureKHR> accelerationStructure) 
{
    MVulkanShaderReflector vertReflector(m_shader->GetVertexShader().GetShader());
    MVulkanShaderReflector fragReflector(m_shader->GetFragmentShader().GetShader());
    PipelineVertexInputStateInfo info = vertReflector.GenerateVertexInputAttributes();

    ShaderReflectorOut resourceOutVert = vertReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindings = resourceOutVert.GetBindings();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
    bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());
    //bindings = RemoveRepeatedBindings(bindings);

    m_descriptorLayouts.Create(m_device.GetDevice(), bindings);
    std::vector<VkDescriptorSetLayout> layouts(m_frameBuffers.size(), m_descriptorLayouts.Get());

    m_descriptorSets.resize(layouts.size());
    for (auto i = 0; i < layouts.size(); i++) {
        m_descriptorSets[i].Create(m_device.GetDevice(), allocator.Get(), layouts[i]);
    }

    m_cbvCount = 0;
    m_separateSamplerCount = 0;
    m_separateImageCount = 0;
    m_combinedImageCount = 0;
    m_asCount = 0;

    for (auto binding = 0; binding < bindings.size(); binding++) {
        switch (bindings[binding].binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            m_cbvCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            m_storageBufferCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        {
            m_combinedImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            m_separateSamplerCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            m_separateImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            m_storageImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        {
            m_asCount++;
            break;
        }
        }
    }

    m_uniformBuffers.resize(m_info.frambufferCount);
    m_storageBuffer.resize(m_storageBufferCount);

    for (auto i = 0; i < m_info.frambufferCount; i++) {
        m_uniformBuffers[i].resize(m_cbvCount);
        for (auto binding = 0; binding < bindings.size(); binding++) {
            switch (bindings[binding].binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                MCBV buffer;
                BufferCreateInfo info;
                info.arrayLength = bindings[binding].binding.descriptorCount;
                info.size = bindings[binding].size;
                buffer.Create(m_device, info);

                //m_uniformBuffers[i].push_back(buffer);
                m_uniformBuffers[i][bindings[binding].binding.binding] = buffer;
                break;
            }
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            {
                //StorageBuffer buffer;
                //BufferCreateInfo info;
                //info.arrayLength = bindings[binding].binding.descriptorCount;
                ////info.size = bindings[binding].size;
                //info.size = storageBuffers[bindings[binding].binding.binding - m_cbvCount];
                //buffer.Create(m_device, info);

                m_storageBuffer[bindings[binding].binding.binding - m_cbvCount] = storageBuffers[bindings[binding].binding.binding - m_cbvCount];
                break;
            }
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            {
                //m_samplerTypes
                break;
            }
            }
        }
    }

    UpdateDescriptorSetWrite(imageViews, storageImageViews, samplers, accelerationStructure);

    m_pipeline.Create(m_device.GetDevice(), m_info.pipelineCreateInfo,
        m_shader->GetVertexShader().GetShaderModule(), m_shader->GetFragmentShader().GetShaderModule(),
        m_renderPass.Get(), info, m_descriptorLayouts.Get(), m_info.imageAttachmentFormats.size(),
        m_shader->GetVertEntryPoint(), m_shader->GetFragEntryPoint());

    m_shader->Clean();
}

void RenderPass::CreateMeshShaderPipeline(MVulkanDescriptorSetAllocator& allocator, std::vector<StorageBuffer> storageBuffers, std::vector<std::vector<VkImageView>> imageViews, std::vector<std::vector<VkImageView>> storageImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructure)
{
    MVulkanShaderReflector meshReflector(m_meshShader->GetMeshShader().GetShader());
    MVulkanShaderReflector fragReflector(m_meshShader->GetFragmentShader().GetShader());
    //PipelineVertexInputStateInfo info = meshReflector.GenerateVertexInputAttributes();

    ShaderReflectorOut resourceOutMesh = meshReflector.GenerateShaderReflactorOut();
    ShaderReflectorOut resourceOutFrag = fragReflector.GenerateShaderReflactorOut();
    std::vector<MVulkanDescriptorSetLayoutBinding> bindings;
    if (m_meshShader->UseTaskShader()) {

        MVulkanShaderReflector taskReflector(m_meshShader->GetTaskShader().GetShader());
        ShaderReflectorOut resourceOutTask = taskReflector.GenerateShaderReflactorOut();

        bindings = resourceOutTask.GetBindings();

        std::vector<MVulkanDescriptorSetLayoutBinding> bindingsMesh = resourceOutMesh.GetBindings();
        std::vector<MVulkanDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
        bindings.insert(bindings.end(), bindingsMesh.begin(), bindingsMesh.end());
        bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());
    }
    else {
        bindings = resourceOutMesh.GetBindings();
        std::vector<MVulkanDescriptorSetLayoutBinding> bindingsFrag = resourceOutFrag.GetBindings();
        bindings.insert(bindings.end(), bindingsFrag.begin(), bindingsFrag.end());
    }
    //bindings = RemoveRepeatedBindings(bindings);

    m_descriptorLayouts.Create(m_device.GetDevice(), bindings);
    std::vector<VkDescriptorSetLayout> layouts(m_frameBuffers.size(), m_descriptorLayouts.Get());

    m_descriptorSets.resize(layouts.size());
    for (auto i = 0; i < layouts.size(); i++) {
        m_descriptorSets[i].Create(m_device.GetDevice(), allocator.Get(), layouts[i]);
    }

    m_cbvCount = 0;
    m_separateSamplerCount = 0;
    m_separateImageCount = 0;
    m_combinedImageCount = 0;
    m_asCount = 0;

    for (auto binding = 0; binding < bindings.size(); binding++) {
        switch (bindings[binding].binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            m_cbvCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            m_storageBufferCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        {
            m_combinedImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            m_separateSamplerCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            m_separateImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            m_storageImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        {
            m_asCount++;
            break;
        }
        }
    }

    m_uniformBuffers.resize(m_info.frambufferCount);
    m_storageBuffer.resize(m_storageBufferCount);

    for (auto i = 0; i < m_info.frambufferCount; i++) {
        m_uniformBuffers[i].resize(m_cbvCount);
        for (auto binding = 0; binding < bindings.size(); binding++) {
            switch (bindings[binding].binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                MCBV buffer;
                BufferCreateInfo info;
                info.arrayLength = bindings[binding].binding.descriptorCount;
                info.size = bindings[binding].size;
                buffer.Create(m_device, info);

                //m_uniformBuffers[i].push_back(buffer);
                m_uniformBuffers[i][bindings[binding].binding.binding] = buffer;
                break;
            }
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            {
                //StorageBuffer buffer;
                //BufferCreateInfo info;
                //info.arrayLength = bindings[binding].binding.descriptorCount;
                ////info.size = bindings[binding].size;
                //info.size = storageBuffers[bindings[binding].binding.binding - m_cbvCount];
                //buffer.Create(m_device, info);

                m_storageBuffer[bindings[binding].binding.binding - m_cbvCount] = storageBuffers[bindings[binding].binding.binding - m_cbvCount];
                break;
            }
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            {
                //m_samplerTypes
                break;
            }
            }
        }
    }

    UpdateDescriptorSetWrite(imageViews, storageImageViews, samplers, accelerationStructure);

    if (m_meshShader->UseTaskShader()) {
        m_pipeline.Create(m_device.GetDevice(), m_info.pipelineCreateInfo,
            m_meshShader->GetTaskShader().GetShaderModule(), m_meshShader->GetMeshShader().GetShaderModule(), m_meshShader->GetFragmentShader().GetShaderModule(),
            m_renderPass.Get(), m_descriptorLayouts.Get(), m_info.imageAttachmentFormats.size(),
            m_meshShader->GetTaskEntryPoint(), m_meshShader->GetMeshEntryPoint(), m_meshShader->GetFragEntryPoint());
    }
    else {
        m_pipeline.Create(m_device.GetDevice(), m_info.pipelineCreateInfo,
            nullptr, m_meshShader->GetMeshShader().GetShaderModule(), m_meshShader->GetFragmentShader().GetShaderModule(),
            m_renderPass.Get(), m_descriptorLayouts.Get(), m_info.imageAttachmentFormats.size(),
            "", m_meshShader->GetMeshEntryPoint(), m_meshShader->GetFragEntryPoint());
    }

    m_meshShader->Clean();
}

void RenderPass::CreateRenderPass()
{
    RenderPassFormatsInfo gbufferFormats{};
    gbufferFormats.useResolvedRef = m_info.useAttachmentResolve;
    gbufferFormats.imageFormats = m_info.imageAttachmentFormats;
    gbufferFormats.depthFormat = m_info.depthFormat;
    gbufferFormats.initialLayout = m_info.initialLayout;
    gbufferFormats.finalLayout = m_info.finalLayout;
    gbufferFormats.initialDepthLayout = m_info.initialDepthLayout;
    gbufferFormats.finalDepthLayout = m_info.finalDepthLayout;
    gbufferFormats.depthLoadOp = m_info.depthLoadOp;
    gbufferFormats.depthStoreOp = m_info.depthStoreOp;
    gbufferFormats.loadOp = m_info.loadOp;
    gbufferFormats.storeOp = m_info.storeOp;
    gbufferFormats.useDepth = m_info.useDepthBuffer;

    m_renderPass.Create(m_device, gbufferFormats);
}

void RenderPass::CreateFrameBuffers(
    MVulkanSwapchain& swapChain, MVulkanCommandQueue& commandQueue, MGraphicsCommandList& commandList)
{
    commandList.Reset();
    commandList.Begin();

    m_frameBuffers.resize(m_info.frambufferCount);

    for (auto i = 0; i < m_info.frambufferCount; i++) {
        FrameBufferCreateInfo info{};

        info.depthStencilFormat = m_info.depthFormat;
        info.useSwapchainImageViews = m_info.useSwapchainImages;
        info.useAttachmentResolve = m_info.useAttachmentResolve;
        info.renderPass = m_renderPass.Get();
        info.extent = m_info.extent;
        info.imageAttachmentFormats = m_info.imageAttachmentFormats.data();
        info.numAttachments = static_cast<uint32_t>(m_info.imageAttachmentFormats.size());
        info.colorAttachmentResolvedViews = m_info.colorAttachmentResolvedViews;
        //info.useAttachmentResolve = m_info.useAttachmentResolve;
        info.swapChainImageIndex = i;
        info.swapChain = swapChain;

        //info.reuseDepthView = m_info.reuseDepthView;
        info.depthView = m_info.depthView;
        info.useDepthBuffer = m_info.useDepthBuffer;

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

void RenderPass::SetMeshShader(std::shared_ptr<MeshShaderModule> meshShader)
{
    m_meshShader = meshShader;
    m_meshShader->Create(m_device.GetDevice());
}

void RenderPass::SetUBO(uint8_t binding, void* data) {
    if(m_shader != nullptr)
        m_shader->SetUBO(binding, data);
    else 
        m_meshShader->SetUBO(binding, data);
    
}

void RenderPass::TransitionFrameBufferImageLayout(MVulkanCommandQueue& queue, MGraphicsCommandList& commandList, VkImageLayout oldLayout, VkImageLayout newLayout) {
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

void RenderPass::LoadCBV(uint32_t alignment) {
    if (m_shader != nullptr) {
        for (auto i = 0; i < m_info.frambufferCount; i++) {
            for (auto binding = 0; binding < m_cbvCount; binding++) {
                for (auto j = 0; j < m_uniformBuffers[i][binding].GetArrayLength(); j++) {
                    uint32_t offset = j * alignment;
                    m_uniformBuffers[i][binding].UpdateData(offset, (void*)(m_shader->GetData(binding, j)));
                }
            }
        }
    }
    
    if (m_meshShader != nullptr) {
        for (auto i = 0; i < m_info.frambufferCount; i++) {
            for (auto binding = 0; binding < m_cbvCount; binding++) {
                for (auto j = 0; j < m_uniformBuffers[i][binding].GetArrayLength(); j++) {
                    uint32_t offset = j * alignment;
                    m_uniformBuffers[i][binding].UpdateData(offset, (void*)(m_meshShader->GetData(binding, j)));
                }
            }
        }
    }
}

void RenderPass::LoadStorageBuffer(uint32_t alignment)
{
    if (m_shader != nullptr) {
        for (auto binding = 0; binding < m_storageBufferCount; binding++) {
            auto _binding = binding + m_cbvCount;
            for (auto j = 0; j < m_storageBuffer[binding].GetArrayLength(); j++) {
                uint32_t offset = j * alignment;
                m_storageBuffer[binding].UpdateData(offset, (void*)(m_shader->GetData(_binding, j)));
            }
        }
    }

    if (m_meshShader != nullptr) {
        for (auto binding = 0; binding < m_storageBufferCount; binding++) {
            auto _binding = binding + m_cbvCount;
            for (auto j = 0; j < m_storageBuffer[binding].GetArrayLength(); j++) {
                uint32_t offset = j * alignment;
                m_storageBuffer[binding].UpdateData(offset, (void*)(m_meshShader->GetData(_binding, j)));
            }
        }
    }
}

void RenderPass::UpdateCBVData()
{

}

MVulkanDescriptorSet RenderPass::CreateDescriptorSet(MVulkanDescriptorSetAllocator& allocator)
{
    MVulkanDescriptorSet descriptorSet;
    descriptorSet.Create(m_device.GetDevice(), allocator.Get(), m_descriptorLayouts.Get());
    return descriptorSet;
}
