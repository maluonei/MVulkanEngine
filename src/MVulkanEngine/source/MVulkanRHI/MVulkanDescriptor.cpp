#include "MVulkanRHI/MVulkanDescriptor.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>

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
    
    //VkDescriptorSetLayoutCreateInfo layoutInfo{};
    //layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    //layoutInfo.pBindings = bindings2.data();
    //
    //VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout));

    //m_device = device;

    //std::vector<VkDescriptorSetLayoutBinding> bindings2;
    //for (auto i = 0; i < bindings.size(); i++) {
    //    VkDescriptorSetLayoutBinding _binding = bindings[i].binding;
    //    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    //    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //    layoutInfo.bindingCount = static_cast<uint32_t>(1);
    //    layoutInfo.pBindings = &_binding;
    //    if(bindings[i].bindingType == BindingType::Bindless)
    //        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;  // 允许动态更新
    //    
    //    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout));
    //}

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings2.data();
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;  // 允许动态更新
    
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
