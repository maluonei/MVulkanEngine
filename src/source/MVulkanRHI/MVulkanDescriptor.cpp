#include "MVulkanRHI/MVulkanDescriptor.h"
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

    descriptorPools.resize(DescriptorType::DESCRIPORTYPE_NUM);

    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, descriptorPools.data()));
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

void MVulkanDescriptorSetWrite::Update(VkDevice device, std::vector<VkDescriptorSet> sets, std::vector<VkDescriptorBufferInfo> infos, int _MAX_FRAMES_IN_FLIGHT)
{
    for (size_t i = 0; i < _MAX_FRAMES_IN_FLIGHT; i++) {


        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = sets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &(infos[i]);

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}
