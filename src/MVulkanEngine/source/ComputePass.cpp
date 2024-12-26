#include "ComputePass.hpp"
#include "MVulkanRHI/MVulkanShader.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

ComputePass::ComputePass(MVulkanDevice device)
{
	m_device = device;
}

void ComputePass::Create(std::shared_ptr<ComputeShaderModule> shader, MVulkanDescriptorSetAllocator& allocator, 
    std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos,
    std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers)
{
    setShader(shader);
    CreatePipeline(allocator, storageBufferSizes, storageImageCreateInfos, seperateImageViews, samplers);
}

void ComputePass::CreatePipeline(MVulkanDescriptorSetAllocator& allocator, 
    std::vector<uint32_t> storageBufferSizes, std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos,
    std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers)
{
	MVulkanShaderReflector compReflector(m_shader->GetComputeShader().GetShader());

    ShaderReflectorOut resourceOutComp = compReflector.GenerateShaderReflactorOut();

    std::vector<MVulkanDescriptorSetLayoutBinding> bindings = resourceOutComp.GetBindings();

    m_descriptorLayout.Create(m_device.GetDevice(), bindings);

    m_descriptorSet.Create(m_device.GetDevice(), allocator.Get(), m_descriptorLayout.Get());

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
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            m_storageImageCount++;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            m_separateImageCount++;
            break;
        }
        }
    }

     m_constantBuffer.resize(m_cbvCount);
     m_storageBuffer.resize(m_storageBufferCount);
     m_storageImages.resize(m_storageImageCount);

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
             m_constantBuffer[bindings[binding].binding.binding] = buffer;
             break;
         }
         case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
         {
             StorageBuffer buffer;
             BufferCreateInfo info;
             info.arrayLength = bindings[binding].binding.descriptorCount;
             //info.size = bindings[binding].size;
             info.size = storageBufferSizes[binding - m_cbvCount];
             buffer.Create(m_device, info);

             //m_uniformBuffers[i].push_back(buffer);
             m_storageBuffer[bindings[binding].binding.binding - m_cbvCount] = buffer;
             break;
         }
         case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
         {
             //m_samplerTypes
             break;
         }
         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
         {
             auto _binding = bindings[binding].binding.binding - m_cbvCount - m_storageBufferCount - m_separateImageCount;
             m_storageImages[_binding].resize(bindings[binding].binding.descriptorCount);
             for (auto i = 0; i < bindings[binding].binding.descriptorCount; i++) {
                 m_storageImages[_binding][i] = std::make_shared<MVulkanTexture>();

                 //MVulkanTexture texture;
                 ImageCreateInfo imageInfo;
                 ImageViewCreateInfo viewInfo;
                 imageInfo.arrayLength = bindings[binding].binding.descriptorCount;
                 imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                 imageInfo.width = storageImageCreateInfos[_binding][i].width;
                 imageInfo.height = storageImageCreateInfos[_binding][i].height;
                 //imageInfo.depth = storageImageCreateInfos[_binding].depth;
                 imageInfo.format = storageImageCreateInfos[_binding][i].format;

                 viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                 viewInfo.format = imageInfo.format;
                 viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
                 viewInfo.baseMipLevel = 0;
                 viewInfo.levelCount = 1;
                 viewInfo.baseArrayLayer = 0;
                 viewInfo.layerCount = 1;

                 Singleton<MVulkanEngine>::instance().CreateImage(m_storageImages[_binding][i], imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
             }
             
             break;
         }
         }
     }

     UpdateDescriptorSetWrite(seperateImageViews, samplers);
     
     m_pipeline.Create(m_device.GetDevice(), m_shader->GetComputeShader().GetShaderModule(), m_descriptorLayout.Get());

     m_shader->Clean();
}

void ComputePass::UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers)
{
    MVulkanDescriptorSetWrite write;

    std::vector<std::vector<VkDescriptorImageInfo>>     combinedImageInfos(0);
    std::vector<std::vector<VkDescriptorImageInfo>>     separateImageInfos(m_separateImageCount);
    std::vector<std::vector<VkDescriptorImageInfo>>     storageImageInfos(m_storageImageCount);
    std::vector<VkDescriptorImageInfo>                  separateSamplerInfos(m_separateSamplerCount);
    std::vector<std::vector<VkDescriptorBufferInfo>>    constantBufferInfos(m_cbvCount);
    std::vector<std::vector<VkDescriptorBufferInfo>>    storageBufferInfos(m_storageBufferCount);


    for (auto binding = 0; binding < m_separateImageCount; binding++) {
        separateImageInfos[binding].resize(seperateImageViews[binding].size());
        for (auto j = 0; j < seperateImageViews[binding].size(); j++) {
            separateImageInfos[binding][j].sampler = nullptr;
            separateImageInfos[binding][j].imageView = seperateImageViews[binding][j];
            separateImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    for (auto binding = 0; binding < m_storageImageCount; binding++) {
        storageImageInfos[binding].resize(1);
        for (auto j = 0; j < 1; j++) {
            storageImageInfos[binding][j].sampler = nullptr;
            storageImageInfos[binding][j].imageView = m_storageImages[binding][j]->GetImageView();
            storageImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        }
    }

    for (auto binding = 0; binding < m_separateSamplerCount; binding++) {
        separateSamplerInfos[binding].sampler = samplers[binding];
        separateSamplerInfos[binding].imageView = nullptr;
        separateSamplerInfos[binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }


    for (auto binding = 0; binding < m_cbvCount; binding++) {
        constantBufferInfos[binding].resize(m_constantBuffer[binding].GetArrayLength());
        for (auto j = 0; j < m_constantBuffer[binding].GetArrayLength(); j++) {
            constantBufferInfos[binding][j].buffer = m_constantBuffer[binding].GetBuffer();
            constantBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_constantBuffer[binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
            constantBufferInfos[binding][j].range = m_constantBuffer[binding].GetBufferSize();
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

    write.Update(m_device.GetDevice(), m_descriptorSet.Get(), 
        constantBufferInfos, storageBufferInfos, 
        combinedImageInfos, separateImageInfos, storageImageInfos, separateSamplerInfos);
}

StorageBuffer ComputePass::GetStorageBufferByBinding(uint32_t binding)
{
    return m_storageBuffer[binding - m_cbvCount];
}

//void ComputePass::LoadConstantAndStorageBuffer(uint32_t alignment)
//{
//    for (auto binding = 0; binding < m_cbvCount; binding++) {
//        for (auto j = 0; j < m_constantBuffer[binding].GetArrayLength(); j++) {
//            uint32_t offset = j * alignment;
//            m_constantBuffer[binding].UpdateData(offset, (void*)(m_shader->GetData(binding, j)));
//        }
//    }
//}

void ComputePass::LoadConstantBuffer(uint32_t alignment)
{
    for (auto binding = 0; binding < m_cbvCount; binding++) {
        for (auto j = 0; j < m_constantBuffer[binding].GetArrayLength(); j++) {
            uint32_t offset = j * alignment;
            m_constantBuffer[binding].UpdateData(offset, (void*)(m_shader->GetData(binding, j)));
        }
    }
}

void ComputePass::LoadStorageBuffer(uint32_t alignment)
{
    for (auto binding = 0; binding < m_storageBufferCount; binding++) {
        auto _binding = binding + m_cbvCount;
        for (auto j = 0; j < m_storageBuffer[binding].GetArrayLength(); j++) {
            uint32_t offset = j * alignment;
            m_storageBuffer[binding].UpdateData(offset, (void*)(m_shader->GetData(_binding, j)));
        }
    }
}

void ComputePass::setShader(std::shared_ptr<ComputeShaderModule> shader)
{
    m_shader = shader;
    m_shader->Create(m_device.GetDevice());
}
