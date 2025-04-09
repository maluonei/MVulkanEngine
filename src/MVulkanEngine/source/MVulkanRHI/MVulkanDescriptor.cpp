#include "MVulkanRHI/MVulkanDescriptor.hpp"
#include "Managers/ShaderManager.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>

//VkDescriptorType ResourceType2VkDescriptorType(const ResourceType type)
//{
//    switch (type)
//    {
//	    case ResourceType_ConstantBuffer:
//		   return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//	    case ResourceType_StorageBuffer:
//		   return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//        case ResourceType_SampledImage:
//            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
//	    case ResourceType_StorageImage:
//		   return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; 
//	    case ResourceType_CombinedImageSampler:
//		   return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        case ResourceType_Sampler:
//            return VK_DESCRIPTOR_TYPE_SAMPLER;
//	    case ResourceType_AccelerationStructure:
//		   return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
//		default:
//			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
//    }
//}
//
//PassResources PassResources::SetBufferResource(
//    int binding, int set, int resourceCount,
//    MCBV buffer, int offset, int range
//) {
//    PassResources resource;
//    resource.m_type = ResourceType_ConstantBuffer;
//    resource.m_binding = binding;
//    resource.m_set = set;
//    resource.m_resourceCount = resourceCount;
//    resource.m_buffer = buffer.GetBuffer();
//    resource.m_offset = offset;
//    resource.m_range = range;
//
//    return resource;
//}
//
//PassResources PassResources::SetBufferResource(std::string name, int binding, int set, int resourceCount)
//{
//    PassResources resource;
//
//    auto& buffer = Singleton<ShaderResourceManager>::instance().GetBuffer(name);
//
//    resource.m_type = ResourceType_ConstantBuffer;
//    resource.m_binding = binding;
//    resource.m_set = set;
//    resource.m_resourceCount = resourceCount;
//    resource.m_buffer = buffer.GetBuffer();
//    resource.m_offset = 0;
//    resource.m_range = buffer.GetBufferSize();
//
//    return resource;
//}
//
//PassResources PassResources::SetBufferResource(
//    int binding, int set, int resourceCount,
//    StorageBuffer buffer, int offset, int range
//) {
//    PassResources resource;
//    resource.m_type = ResourceType_StorageBuffer;
//    resource.m_binding = binding;
//    resource.m_set = set;
//    resource.m_resourceCount = resourceCount;
//    resource.m_buffer = buffer.GetBuffer();
//    resource.m_offset = offset;
//    resource.m_range = range;
//
//    return resource;
//}
//
//PassResources PassResources::SetSampledImageResource(
//    int binding, int set, int resourceCount,
//    VkImageView view
//) {
//    PassResources resource;
//    resource.m_type = ResourceType_SampledImage;
//    resource.m_binding = binding;
//    resource.m_set = set;
//    resource.m_resourceCount = resourceCount;
//    resource.m_view = view;
//
//    return resource;
//}
//
//
//PassResources PassResources::SetStorageImageResource(
//    int binding, int set, int resourceCount,
//    VkImageView view
//) {
//    PassResources resource;
//    resource.m_type = ResourceType_StorageImage;
//    resource.m_binding = binding;
//    resource.m_set = set;
//    resource.m_resourceCount = resourceCount;
//    resource.m_view = view;
//
//    return resource;
//}
//
//
//PassResources PassResources::SetSamplerResource(
//    int binding, int set, int resourceCount,
//    VkSampler sampler
//) {
//    PassResources resource;
//    resource.m_type = ResourceType_Sampler;
//    resource.m_binding = binding;
//    resource.m_set = set;
//    resource.m_resourceCount = resourceCount;
//    resource.m_sampler = sampler;
//
//    return resource;
//}
//
//PassResources PassResources::SetCombinedImageSamplerResource(
//    int binding, int set, int resourceCount,
//    VkAccelerationStructureKHR* accel
//) {
//    PassResources resource;
//    resource.m_type = ResourceType_AccelerationStructure;
//    resource.m_binding = binding;
//    resource.m_set = set;
//    resource.m_resourceCount = resourceCount;
//    resource.m_acc = accel;
//
//    return resource;
//}
//
//
//PassResources PassResources::SetCombinedImageSamplerResource(
//    int binding, int set, int resourceCount,
//    VkImageView view, VkSampler sampler
//) {
//    PassResources resource;
//    resource.m_type = ResourceType_CombinedImageSampler;
//    resource.m_binding = binding;
//    resource.m_set = set;
//    resource.m_resourceCount = resourceCount;
//    resource.m_view = view;
//    resource.m_sampler = sampler;
//
//    return resource;
//}


MVulkanDescriptorSetAllocator::MVulkanDescriptorSetAllocator()
{

}

void MVulkanDescriptorSetAllocator::Create(VkDevice device)
{
    m_device = device;

    std::vector<VkDescriptorPoolSize> poolSizes(DescriptorType::DESCRIPORTYPE_NUM);
    for (int type = 0; type < DescriptorType::DESCRIPORTYPE_NUM; type++) {
        poolSizes[type].type = DescriptorType2VkDescriptorType((DescriptorType)type);
        poolSizes[type].descriptorCount = static_cast<uint32_t>(64);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(1024);
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;

    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool));
}


void MVulkanDescriptorSetAllocator::Clean()
{
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
}

MVulkanDescriptorSetLayouts::MVulkanDescriptorSetLayouts(VkDescriptorSetLayout _layout):m_layout(_layout){}

void MVulkanDescriptorSetLayouts::Create(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings)
{
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout));
}

void MVulkanDescriptorSetLayouts::Create(VkDevice device, std::vector<MVulkanDescriptorSetLayoutBinding> bindings)
{
    m_device = device;
    //
    std::vector<VkDescriptorSetLayoutBinding> bindings2;
    for (auto i = 0; i < bindings.size(); i++) {
        bindings2.push_back(bindings[i].binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings2.data();
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;  // ������̬����
    
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout));
}

void MVulkanDescriptorSetLayouts::Clean()
{
    vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
}

//MVulkanDescriptorSet::MVulkanDescriptorSet(VkDescriptorSet _set):m_set(_set){}

void MVulkanDescriptorSet::Create(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
    m_device = device;
    m_pool = pool;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &m_set));
}

void MVulkanDescriptorSet::Clean()
{
    VK_CHECK_RESULT(vkFreeDescriptorSets(m_device, m_pool, 1, &m_set));
}

//void MVulkanDescriptorSetWrite::Update(
//    VkDevice device,
//    VkDescriptorSet set,
//    std::vector<std::vector<VkDescriptorBufferInfo>> constantBufferInfos,
//    std::vector<std::vector<VkDescriptorBufferInfo>> storageBufferInfos,
//    std::vector<std::vector<VkDescriptorImageInfo>> combinedImageInfos,
//    std::vector<std::vector<VkDescriptorImageInfo>> seperateImageInfos,
//    std::vector<std::vector<VkDescriptorImageInfo>> storageImageInfos,
//    std::vector<VkDescriptorImageInfo> seperateSamplers,
//    std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    auto constantBufferInfoCount = constantBufferInfos.size();
//    auto storageBufferInfoCount = storageBufferInfos.size();
//    auto combinedImageInfosCount = combinedImageInfos.size();
//    auto separateImageInfosCount = seperateImageInfos.size();
//    auto storageImageInfosCount = storageImageInfos.size();
//    auto separateSamplerInfosCount = seperateSamplers.size();
//    auto accelerationStructuresCount = accelerationStructures.size();
//
//    std::vector<VkWriteDescriptorSet> descriptorWrite(constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount + separateSamplerInfosCount + accelerationStructuresCount);
//    for (auto binding = 0; binding < constantBufferInfoCount; binding++) {
//        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrite[binding].dstSet = set;
//        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
//        descriptorWrite[binding].dstArrayElement = 0;
//        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        descriptorWrite[binding].descriptorCount = constantBufferInfos[binding].size();
//        descriptorWrite[binding].pBufferInfo = &(constantBufferInfos[binding][0]);
//    }
//
//    for (auto binding = constantBufferInfoCount; binding < constantBufferInfoCount + storageBufferInfoCount; binding++) {
//        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrite[binding].dstSet = set;
//        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
//        descriptorWrite[binding].dstArrayElement = 0;
//        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//        descriptorWrite[binding].descriptorCount = storageBufferInfos[binding - constantBufferInfoCount].size();
//        descriptorWrite[binding].pBufferInfo = &(storageBufferInfos[binding - constantBufferInfoCount][0]);
//    }
//
//    for (auto binding = constantBufferInfoCount + storageBufferInfoCount; binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount; binding++) {
//        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrite[binding].dstSet = set;
//        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
//        descriptorWrite[binding].dstArrayElement = 0;
//        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        descriptorWrite[binding].descriptorCount = combinedImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount].size();
//        descriptorWrite[binding].pImageInfo = &(combinedImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount][0]);
//    }
//
//    for (auto binding = constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount; binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount; binding++) {
//        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrite[binding].dstSet = set;
//        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
//        descriptorWrite[binding].dstArrayElement = 0;
//        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
//        descriptorWrite[binding].descriptorCount = seperateImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount].size();
//        descriptorWrite[binding].pImageInfo = &(seperateImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount][0]);
//    }
//
//    for (auto binding = constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount; binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount; binding++) {
//        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrite[binding].dstSet = set;
//        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
//        descriptorWrite[binding].dstArrayElement = 0;
//        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//        descriptorWrite[binding].descriptorCount = storageImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount - separateImageInfosCount].size();
//        descriptorWrite[binding].pImageInfo = &(storageImageInfos[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount - separateImageInfosCount][0]);
//    }
//
//    for (auto binding = constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount; binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + separateSamplerInfosCount + storageImageInfosCount; binding++) {
//        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrite[binding].dstSet = set;
//        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
//        descriptorWrite[binding].dstArrayElement = 0;
//        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
//        descriptorWrite[binding].descriptorCount = 1;
//        descriptorWrite[binding].pImageInfo = &seperateSamplers[binding - constantBufferInfoCount - storageBufferInfoCount - combinedImageInfosCount - separateImageInfosCount - storageImageInfosCount];
//    }
//
//    std::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureWrites(accelerationStructuresCount);
//    for (auto binding = 
//        constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount + separateSamplerInfosCount; 
//        binding < constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + separateSamplerInfosCount + storageImageInfosCount + accelerationStructuresCount; 
//        binding++) 
//    {
//        auto idx = binding - (constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount + separateSamplerInfosCount);
//        accelerationStructureWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
//        accelerationStructureWrites[idx].accelerationStructureCount = 1;
//        accelerationStructureWrites[idx].pAccelerationStructures = &accelerationStructures[idx];
//        
//        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrite[binding].dstSet = set;
//        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
//        descriptorWrite[binding].dstArrayElement = 0;
//        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
//        descriptorWrite[binding].descriptorCount = 1;
//        descriptorWrite[binding].pNext = &accelerationStructureWrites[idx];
//    }
//
//    vkUpdateDescriptorSets(device, constantBufferInfoCount + storageBufferInfoCount + combinedImageInfosCount + separateImageInfosCount + storageImageInfosCount + separateSamplerInfosCount + accelerationStructuresCount , descriptorWrite.data(), 0, nullptr);
//}

//void MVulkanDescriptorSetWrite::Update(VkDevice device, VkDescriptorSet set, std::vector<PassResources> resources)
//{
//    std::vector<VkWriteDescriptorSet> descriptorWrite(resources.size());
//
//    std::vector<VkDescriptorBufferInfo> bufferInfos;
//    std::vector<VkDescriptorImageInfo> imageInfos;
//    std::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureWrites;
//
//    for (int i=0;i< resources.size();i++)
//    {
//        auto& resource = resources[i];
//
//        descriptorWrite[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrite[i].dstSet = set;
//        descriptorWrite[i].dstBinding = static_cast<uint32_t>(resource.m_binding);
//        descriptorWrite[i].dstArrayElement = 0;
//        descriptorWrite[i].descriptorType = ResourceType2VkDescriptorType(resource.m_type);
//        descriptorWrite[i].descriptorCount = resource.m_resourceCount;
//        //descriptorWrite[i].pImageInfo =
//
//        VkDescriptorBufferInfo bufferInfo{};
//        VkDescriptorImageInfo imageInfo{};
//        VkWriteDescriptorSetAccelerationStructureKHR acclInfo{};
//        switch (resource.m_type)
//        {
//        case ResourceType_ConstantBuffer:
//        case ResourceType_StorageBuffer:
//            bufferInfo.buffer = resource.m_buffer;
//            bufferInfo.offset = resource.m_offset;
//            bufferInfo.range = resource.m_range;
//
//            bufferInfos.emplace_back(bufferInfo);
//            descriptorWrite[i].pBufferInfo = &bufferInfos[bufferInfos.size()-1];
//            break;
//        case ResourceType_SampledImage:
//            imageInfo.imageView = resource.m_view;
//            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//
//            imageInfos.emplace_back(imageInfo);
//            descriptorWrite[i].pImageInfo = &imageInfos[imageInfos.size() - 1];
//            break;
//        case ResourceType_StorageImage:
//            imageInfo.imageView = resource.m_view;
//            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//
//            imageInfos.emplace_back(imageInfo);
//            descriptorWrite[i].pImageInfo = &imageInfos[imageInfos.size() - 1];
//            break;
//        case ResourceType_Sampler:
//            imageInfo.sampler = resource.m_sampler;
//            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//
//            imageInfos.emplace_back(imageInfo);
//            descriptorWrite[i].pImageInfo = &imageInfos[imageInfos.size() - 1];
//            break;
//        case ResourceType_CombinedImageSampler:
//            imageInfo.imageView = resource.m_view;
//            imageInfo.sampler = resource.m_sampler;
//            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//
//            imageInfos.emplace_back(imageInfo);
//            descriptorWrite[i].pImageInfo = &imageInfos[imageInfos.size() - 1];
//            break;
//        case ResourceType_AccelerationStructure:
//            acclInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
//            acclInfo.accelerationStructureCount = 1;
//            acclInfo.pAccelerationStructures = resource.m_acc;
//
//            accelerationStructureWrites.emplace_back(acclInfo);
//            descriptorWrite[i].pNext = &accelerationStructureWrites[imageInfos.size() - 1];
//        }
//        
//        vkUpdateDescriptorSets(device, descriptorWrite.size(), descriptorWrite.data(), 0, nullptr);
//    }
//}
