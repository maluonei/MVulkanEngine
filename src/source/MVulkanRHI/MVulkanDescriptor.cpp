#include "MVulkanRHI/MVulkanDescriptor.hpp"
#include <stdexcept>

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
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(32);

    //descriptorPools.resize(DescriptorType::DESCRIPORTYPE_NUM);

    //VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, descriptorPools.data()));
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
}

void MVulkanDescriptorSetLayouts::Create(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings)
{
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));
}

void MVulkanDescriptorSet::Create(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, int _MAX_FRAMES_IN_FLIGHT)
{
    std::vector<VkDescriptorSetLayout> layouts(_MAX_FRAMES_IN_FLIGHT, layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(_MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    sets.resize(_MAX_FRAMES_IN_FLIGHT);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, sets.data()));
}

void MVulkanDescriptorSetWrite::Update(VkDevice device, std::vector<VkDescriptorSet> sets, std::vector<VkDescriptorBufferInfo> bufferInfos, std::vector<VkDescriptorImageInfo> imageInfos, int _MAX_FRAMES_IN_FLIGHT)
{
    auto bufferInfoCount = bufferInfos.size();
    auto imageInfosCount = imageInfos.size();

    std::vector<std::vector<VkDescriptorBufferInfo>> fullBufferInfos(_MAX_FRAMES_IN_FLIGHT);
    std::vector<std::vector<VkDescriptorImageInfo>> fullImageInfos(_MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < _MAX_FRAMES_IN_FLIGHT; i++) {
        fullBufferInfos[i] = bufferInfos;
        fullImageInfos[i] = imageInfos;
    }

    for (size_t i = 0; i < _MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrite(bufferInfoCount + imageInfosCount);
        for (int j = 0; j < bufferInfoCount; j++) {
            descriptorWrite[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[j].dstSet = sets[i];
            descriptorWrite[j].dstBinding = j;
            descriptorWrite[j].dstArrayElement = 0;
            descriptorWrite[j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite[j].descriptorCount = 1;
            descriptorWrite[j].pBufferInfo = &(fullBufferInfos[i][j]);
        }

        for (int j = bufferInfoCount; j < bufferInfoCount + imageInfosCount; j++) {
            descriptorWrite[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[j].dstSet = sets[i];
            descriptorWrite[j].dstBinding = j;
            descriptorWrite[j].dstArrayElement = 0;
            descriptorWrite[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite[j].descriptorCount = 1;
            descriptorWrite[j].pImageInfo = &(fullImageInfos[i][j - bufferInfoCount]);
        }

        vkUpdateDescriptorSets(device, bufferInfoCount + imageInfosCount, descriptorWrite.data(), 0, nullptr);
    }
}
