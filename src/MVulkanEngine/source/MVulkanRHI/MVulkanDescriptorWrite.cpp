#include "MVulkanRHI/MVulkanDescriptorWrite.hpp"
#include "Managers/ShaderManager.hpp"
//#include "MVulkanRHI/MVulkanBuffer.hpp"


PassResources PassResources::SetBufferResource(
    int binding, int set, 
    MCBV buffer, int offset, int range
) {
    PassResources resource;
    resource.m_type = ResourceType_ConstantBuffer;
    resource.m_binding = binding;
    resource.m_set = set;
    
    resource.m_buffers = { buffer.GetBuffer() };
    resource.m_offset = offset;
    resource.m_range = range;

    return resource;
}

PassResources PassResources::SetBufferResource(std::string name, int binding, int set)
{
    PassResources resource;

    auto buffer = Singleton<ShaderResourceManager>::instance().GetBuffer(name);

    resource.m_type = ResourceType_ConstantBuffer;
    resource.m_binding = binding;
    resource.m_set = set;
    
    resource.m_buffers = { buffer.GetBuffer() };
    resource.m_offset = 0;
    resource.m_range = buffer.GetBufferSize();

    return resource;
}

PassResources PassResources::SetBufferResource(std::string name, int binding, int set, int frameIndex)
{
    PassResources resource;

    auto buffer = Singleton<ShaderResourceManager>::instance().GetBuffer(name, frameIndex);

    resource.m_type = ResourceType_ConstantBuffer;
    resource.m_binding = binding;
    resource.m_set = set;

    resource.m_buffers = { buffer.GetBuffer() };
    resource.m_offset = 0;
    resource.m_range = buffer.GetBufferSize();

    return resource;
}

PassResources PassResources::SetBufferResource(std::string name, std::string shaderResourceName, int binding, int set, int frameIndex)
{
    PassResources resource;

    auto buffer = Singleton<ShaderResourceManager>::instance().GetBuffer(name, shaderResourceName, frameIndex);

    resource.m_type = ResourceType_ConstantBuffer;
    resource.m_binding = binding;
    resource.m_set = set;

    resource.m_buffers = { buffer.GetBuffer() };
    resource.m_offset = 0;
    resource.m_range = buffer.GetBufferSize();

    return resource;
}

PassResources PassResources::SetBufferResource(
    int binding, int set, 
    std::shared_ptr<StorageBuffer> buffer
) {
    PassResources resource;
    resource.m_type = ResourceType_StorageBuffer;
    resource.m_binding = binding;
    resource.m_set = set;
    
    resource.m_buffers = { buffer->GetBuffer() };
    resource.m_offset = 0;
    resource.m_range = buffer->GetBufferSize();

    return resource;
}

PassResources PassResources::SetSampledImageResource(
    int binding, int set,
    std::shared_ptr<MVulkanTexture> texture
) {
    PassResources resource;
    resource.m_type = ResourceType_SampledImage;
    resource.m_binding = binding;
    resource.m_set = set;

    resource.m_textures = { 
        TextureSubResource{
            .m_texture = texture,
            .m_mipLevel = 0,
            .m_mipCount = texture->GetMaxMipCount()}};

    return resource;
}

PassResources PassResources::SetSampledImageResource(
    int binding, int set,
    TextureSubResource texture
) {
    PassResources resource;
    resource.m_type = ResourceType_SampledImage;
    resource.m_binding = binding;
    resource.m_set = set;
    
    resource.m_textures = {
        texture };

    return resource;
}

PassResources PassResources::SetSampledImageResource(
    int binding, 
    int set, 
    std::vector<TextureSubResource> textures)
{
    PassResources resource;
    resource.m_type = ResourceType_SampledImage;
    resource.m_binding = binding;
    resource.m_set = set;

    resource.m_textures = textures;

    return resource;
}

PassResources PassResources::SetSampledImageResource(
    int binding,
    int set,
    std::vector<std::shared_ptr<MVulkanTexture>> textures)
{
    PassResources resource;
    resource.m_type = ResourceType_SampledImage;
    resource.m_binding = binding;
    resource.m_set = set;

    std::vector<TextureSubResource> texs;
    
    auto numTextures = textures.size();
    for (auto i = 0; i < numTextures; i++) {
        texs.push_back(
            TextureSubResource{
                .m_texture = textures[i],
                .m_mipLevel = 0,
                .m_mipCount = textures[i]->GetMaxMipCount() }
        );
    }

    resource.m_textures = texs;

    return resource;
}


PassResources PassResources::SetStorageImageResource(
    int binding, int set, 
    TextureSubResource texture
) {
    PassResources resource;
    resource.m_type = ResourceType_StorageImage;
    resource.m_binding = binding;
    resource.m_set = set;
    
    resource.m_textures = {texture };


    //resource.m_views = { view };

    return resource;
}

PassResources PassResources::SetStorageImageResource(
    int binding, int set,
    std::shared_ptr<MVulkanTexture> texture
) {
    PassResources resource;
    resource.m_type = ResourceType_StorageImage;
    resource.m_binding = binding;
    resource.m_set = set;

    resource.m_textures = {
    TextureSubResource{
        .m_texture = texture,
        .m_mipLevel = 0,
        .m_mipCount = texture->GetMaxMipCount()} };

    return resource;
}

PassResources PassResources::SetStorageImageResource(
    int binding,
    int set, 
    std::vector<TextureSubResource> textures)
{
    PassResources resource;
    resource.m_type = ResourceType_StorageImage;
    resource.m_binding = binding;
    resource.m_set = set;

    resource.m_textures = textures;



    return resource;
}


PassResources PassResources::SetSamplerResource(
    int binding, int set, 
    VkSampler sampler
) {
    PassResources resource;
    resource.m_type = ResourceType_Sampler;
    resource.m_binding = binding;
    resource.m_set = set;
    
    resource.m_samplers = { sampler };

    return resource;
}

PassResources PassResources::SetAccelerationStructureResource(
    int binding, int set, 
    VkAccelerationStructureKHR* accel
) {
    PassResources resource;
    resource.m_type = ResourceType_AccelerationStructure;
    resource.m_binding = binding;
    resource.m_set = set;
    
    resource.m_accs = { accel };

    return resource;
}


PassResources PassResources::SetCombinedImageSamplerResource(
    int binding, int set, 
    TextureSubResource texture, VkSampler sampler
) {
    PassResources resource;
    resource.m_type = ResourceType_CombinedImageSampler;
    resource.m_binding = binding;
    resource.m_set = set;
    
    resource.m_textures = { texture };
    resource.m_samplers = { sampler };

    return resource;
}

void MVulkanDescriptorSetWrite::Update(
    VkDevice device,
    VkDescriptorSet set,
    std::vector<std::vector<VkDescriptorBufferInfo>> constantBufferInfos,
    std::vector<std::vector<VkDescriptorBufferInfo>> storageBufferInfos,
    std::vector<std::vector<VkDescriptorImageInfo>> combinedImageInfos,
    std::vector<std::vector<VkDescriptorImageInfo>> seperateImageInfos,
    std::vector<std::vector<VkDescriptorImageInfo>> storageImageInfos,
    std::vector<VkDescriptorImageInfo> seperateSamplers,
    std::vector<VkAccelerationStructureKHR> accelerationStructures)
{
    auto constantBufferInfoCount = constantBufferInfos.size();
    auto storageBufferInfoCount = storageBufferInfos.size();
    auto combinedImageInfosCount = combinedImageInfos.size();
    auto separateImageInfosCount = seperateImageInfos.size();
    auto storageImageInfosCount = storageImageInfos.size();
    auto separateSamplerInfosCount = seperateSamplers.size();
    auto accelerationStructuresCount = accelerationStructures.size();

    std::vector<VkWriteDescriptorSet> descriptorWrite(constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount + separateSamplerInfosCount + accelerationStructuresCount);
    for (auto binding = 0; binding < constantBufferInfoCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite[binding].descriptorCount = constantBufferInfos[binding].size();
        descriptorWrite[binding].pBufferInfo = &(constantBufferInfos[binding][0]);
    }

    for (auto binding = constantBufferInfoCount; binding < constantBufferInfoCount + storageBufferInfoCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite[binding].descriptorCount = storageBufferInfos[binding - constantBufferInfoCount].size();
        descriptorWrite[binding].pBufferInfo = &(storageBufferInfos[binding - constantBufferInfoCount][0]);
    }

    for (auto binding = constantBufferInfoCount + storageBufferInfoCount; binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite[binding].descriptorCount = combinedImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount].size();
        descriptorWrite[binding].pImageInfo = &(combinedImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount][0]);
    }

    for (auto binding = constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount; binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite[binding].descriptorCount = seperateImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount].size();
        descriptorWrite[binding].pImageInfo = &(seperateImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount][0]);
    }

    for (auto binding = constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount; binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrite[binding].descriptorCount = storageImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount - separateImageInfosCount].size();
        descriptorWrite[binding].pImageInfo = &(storageImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount - separateImageInfosCount][0]);
    }

    for (auto binding = constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount; binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + separateSamplerInfosCount + storageImageInfosCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite[binding].descriptorCount = 1;
        descriptorWrite[binding].pImageInfo = &seperateSamplers[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount - separateImageInfosCount - storageImageInfosCount];
    }

    std::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureWrites(accelerationStructuresCount);
    for (auto binding = 
        constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount + separateSamplerInfosCount; 
        binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + separateSamplerInfosCount + storageImageInfosCount + accelerationStructuresCount; 
        binding++) 
    {
        auto idx = binding - (constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount + separateSamplerInfosCount);
        accelerationStructureWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureWrites[idx].accelerationStructureCount = 1;
        accelerationStructureWrites[idx].pAccelerationStructures = &accelerationStructures[idx];
        
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        descriptorWrite[binding].descriptorCount = 1;
        descriptorWrite[binding].pNext = &accelerationStructureWrites[idx];
    }

    vkUpdateDescriptorSets(device, constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount + separateSamplerInfosCount + accelerationStructuresCount , descriptorWrite.data(), 0, nullptr);
}


void MVulkanDescriptorSetWrite::Update(VkDevice device, VkDescriptorSet set, std::vector<PassResources> resources)
{
    std::vector<VkWriteDescriptorSet> descriptorWrite(resources.size());

    std::vector<std::vector<VkDescriptorBufferInfo>> r_bufferInfos;
    std::vector<std::vector<VkDescriptorImageInfo>> r_imageInfos;
    std::vector<std::vector<VkWriteDescriptorSetAccelerationStructureKHR>> r_accelerationStructureWrites;

    //for (int i = 0; i < resources.size(); i++)
    //{
    //    auto& resource = resources[i];
    //
    //
    //}

    for (int i=0;i < resources.size();i++)
    {
        auto& resource = resources[i];

        descriptorWrite[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[i].dstSet = set;
        descriptorWrite[i].dstBinding = static_cast<uint32_t>(resource.m_binding);
        descriptorWrite[i].dstArrayElement = 0;
        descriptorWrite[i].descriptorType = ResourceType2VkDescriptorType(resource.m_type);

        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkWriteDescriptorSetAccelerationStructureKHR> acclInfos;
        switch (resource.m_type)
        {
        case ResourceType_ConstantBuffer:
        case ResourceType_StorageBuffer:
            descriptorWrite[i].descriptorCount = resource.m_buffers.size();
            for (int j = 0; j < descriptorWrite[i].descriptorCount; j++) {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = resource.m_buffers[j];
                bufferInfo.offset = resource.m_offset;
                bufferInfo.range = resource.m_range;
                bufferInfos.emplace_back(bufferInfo);
            }
            r_bufferInfos.push_back(bufferInfos);

            descriptorWrite[i].pBufferInfo = &r_bufferInfos[r_bufferInfos.size()-1][0];
            break;
        case ResourceType_SampledImage:
            descriptorWrite[i].descriptorCount = resource.m_textures.size();
            for (int j = 0; j < descriptorWrite[i].descriptorCount; j++) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageView = resource.m_textures[j].GetImageView();
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos.emplace_back(imageInfo);
            }
            r_imageInfos.push_back(imageInfos);

            descriptorWrite[i].pImageInfo = &r_imageInfos[r_imageInfos.size()-1][0];
            break;
        case ResourceType_StorageImage:
            descriptorWrite[i].descriptorCount = resource.m_textures.size();
            for (int j = 0; j < descriptorWrite[i].descriptorCount; j++) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageView = resource.m_textures[j].GetImageView();
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfos.emplace_back(imageInfo);
            }
            r_imageInfos.push_back(imageInfos);

            descriptorWrite[i].pImageInfo = &r_imageInfos[r_imageInfos.size() - 1][0];
            //descriptorWrite[i].pImageInfo = &imageInfos[0];
            break;
        case ResourceType_Sampler:
            descriptorWrite[i].descriptorCount = resource.m_samplers.size();
            for (int j = 0; j < descriptorWrite[i].descriptorCount; j++) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = resource.m_samplers[j];
                //imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfos.emplace_back(imageInfo);
            }
            r_imageInfos.push_back(imageInfos);

            descriptorWrite[i].pImageInfo = &r_imageInfos[r_imageInfos.size() - 1][0];
            //descriptorWrite[i].pImageInfo = &imageInfos[0];
            break;
        case ResourceType_CombinedImageSampler:
            descriptorWrite[i].descriptorCount = resource.m_textures.size();
            for (int j = 0; j < descriptorWrite[i].descriptorCount; j++) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageView = resource.m_textures[j].GetImageView();
                imageInfo.sampler = resource.m_samplers[j];
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos.emplace_back(imageInfo);
            }
            r_imageInfos.push_back(imageInfos);

            descriptorWrite[i].pImageInfo = &r_imageInfos[r_imageInfos.size() - 1][0];
            //descriptorWrite[i].pImageInfo = &imageInfos[0];
            break;
        case ResourceType_AccelerationStructure:
            descriptorWrite[i].descriptorCount = resource.m_accs.size();
            for (int j = 0; j < descriptorWrite[i].descriptorCount; j++) {
                VkWriteDescriptorSetAccelerationStructureKHR acclInfo{};
                acclInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                acclInfo.accelerationStructureCount = 1;
                acclInfo.pAccelerationStructures = resource.m_accs[j];

                acclInfos.emplace_back(acclInfo);
            }
            r_accelerationStructureWrites.push_back(acclInfos);

            descriptorWrite[i].pNext = &r_accelerationStructureWrites[r_accelerationStructureWrites.size() - 1][0];
            //accelerationStructureWrites.emplace_back(acclInfo);
            //descriptorWrite[i].pNext = &acclInfos[0];
        }
    }
    vkUpdateDescriptorSets(device, descriptorWrite.size(), descriptorWrite.data(), 0, nullptr);
}
