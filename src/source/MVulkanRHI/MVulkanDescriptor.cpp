#include "MVulkanRHI/MVulkanDescriptor.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>

MVulkanDescriptorSetAllocator::MVulkanDescriptorSetAllocator()
{

}

void MVulkanDescriptorSetAllocator::Create(VkDevice device)
{
    std::vector<VkDescriptorPoolSize> poolSizes(DescriptorType::DESCRIPORTYPE_NUM);
    for (int type = 0; type < DescriptorType::DESCRIPORTYPE_NUM; type++) {
        poolSizes[type].type = DescriptorType2VkDescriptorType((DescriptorType)type);
        poolSizes[type].descriptorCount = static_cast<uint32_t>(32);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(32);

    //descriptorPools.resize(DescriptorType::DESCRIPORTYPE_NUM);

    //VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, descriptorPools.data()));
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
}

MVulkanDescriptorSetLayouts::MVulkanDescriptorSetLayouts(VkDescriptorSetLayout _layout):layout(_layout){}

void MVulkanDescriptorSetLayouts::Create(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings)
{
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));
}

void MVulkanDescriptorSetLayouts::Create(VkDevice device, std::vector<MVulkanDescriptorSetLayoutBinding> bindings)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings2;
    for (auto i = 0; i < bindings.size(); i++) {
        bindings2.push_back(bindings[i].binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings2.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));
}

MVulkanDescriptorSet::MVulkanDescriptorSet(VkDescriptorSet _set):set(_set){}

void MVulkanDescriptorSet::Create(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    //sets.resize(_MAX_FRAMES_IN_FLIGHT);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &set));
}

std::vector<MVulkanDescriptorSet> MVulkanDescriptorSet::CreateDescriptorSets(VkDevice device, VkDescriptorPool pool, std::vector<VkDescriptorSetLayout> layouts)
{
    std::vector<VkDescriptorSet> descriptorSets(layouts.size());

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    //sets.resize(_MAX_FRAMES_IN_FLIGHT);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));

    std::vector<MVulkanDescriptorSet> sets(layouts.size());
    for (auto i = 0; i < descriptorSets.size(); i++) {
        sets[i] = MVulkanDescriptorSet(descriptorSets[i]);
    }

    return sets;
}

void MVulkanDescriptorSetWrite::Update(
    VkDevice device,
    VkDescriptorSet set,
    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos,
    std::vector<std::vector<VkDescriptorImageInfo>> combinedImageInfos,
    std::vector<std::vector<VkDescriptorImageInfo>> seperateImageInfos,
    std::vector<VkDescriptorImageInfo> seperateSamplers)
{
    auto bufferInfoCount = bufferInfos.size();
    auto combinedImageInfosCount = combinedImageInfos.size();
    auto separateImageInfosCount = seperateImageInfos.size();
    auto separateSamplerInfosCount = seperateSamplers.size();

    std::vector<VkWriteDescriptorSet> descriptorWrite(bufferInfoCount + combinedImageInfosCount + separateImageInfosCount + separateSamplerInfosCount);
    for (auto binding = 0; binding < bufferInfoCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite[binding].descriptorCount = bufferInfos[binding].size();
        descriptorWrite[binding].pBufferInfo = &(bufferInfos[binding][0]);
    }

    for (auto binding = bufferInfoCount; binding < bufferInfoCount + combinedImageInfosCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite[binding].descriptorCount = combinedImageInfos[binding - bufferInfoCount].size();
        descriptorWrite[binding].pImageInfo = &(combinedImageInfos[binding - bufferInfoCount][0]);
    }

    for (auto binding = bufferInfoCount + combinedImageInfosCount; binding < bufferInfoCount + combinedImageInfosCount + separateImageInfosCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite[binding].descriptorCount = seperateImageInfos[binding - bufferInfoCount - combinedImageInfosCount].size();
        descriptorWrite[binding].pImageInfo = &(seperateImageInfos[binding - bufferInfoCount - combinedImageInfosCount][0]);
    }

    for (auto binding = bufferInfoCount + combinedImageInfosCount + separateImageInfosCount; binding < bufferInfoCount + combinedImageInfosCount + separateImageInfosCount + separateSamplerInfosCount; binding++) {
        descriptorWrite[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[binding].dstSet = set;
        descriptorWrite[binding].dstBinding = static_cast<uint32_t>(binding);
        descriptorWrite[binding].dstArrayElement = 0;
        descriptorWrite[binding].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite[binding].descriptorCount = 1;
        descriptorWrite[binding].pImageInfo = &seperateSamplers[binding - bufferInfoCount - combinedImageInfosCount - separateImageInfosCount];
    }

    vkUpdateDescriptorSets(device, bufferInfoCount + combinedImageInfosCount + separateImageInfosCount + separateSamplerInfosCount, descriptorWrite.data(), 0, nullptr);
}
