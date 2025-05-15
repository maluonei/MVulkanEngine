#include "ComputePass.hpp"
#include "MVulkanRHI/MVulkanShader.hpp"
#include "Shaders/ShaderModule.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Managers/ShaderManager.hpp"

ComputePass::ComputePass(MVulkanDevice device)
{
	m_device = device;
}

void ComputePass::Clean()
{
    for (auto i = 0; i < m_constantBuffer.size(); i++) {
        m_constantBuffer[i].Clean();
    }
    m_constantBuffer.clear();

    for (auto i = 0; i < m_storageBuffer.size(); i++) {
        m_storageBuffer[i].Clean();
    }
    m_storageBuffer.clear();

    for (auto i = 0; i < m_storageImages.size(); i++) {
        for (auto j = 0; j < m_storageImages[i].size(); j++) {
            m_storageImages[i][j]->Clean();
        }
        m_storageImages[i].clear();
    }
    m_storageImages.clear();

    //m_descriptorLayout.Clean();
    for (auto& descriptorLayout : m_descriptorLayouts) {
        descriptorLayout.Clean();
    }

    //m_descriptorSet.Clean();
    for (auto& it : m_descriptorSets) {
        it.second.Clean();
    }

    m_pipeline.Clean();
}

void ComputePass::Create(std::shared_ptr<ComputeShaderModule> shader, MVulkanDescriptorSetAllocator& allocator)
{
    setShader(shader);
    CreatePipeline(allocator);
}

void ComputePass::UpdateDescriptorSetWrite(std::vector<PassResources> resources)
{
    std::sort(resources.begin(), resources.end(), [](PassResources a, PassResources b) {
        if (a.m_set == b.m_set)
        {
            return a.m_binding < b.m_binding;
        }
        return a.m_set < b.m_set; // ½µÐò
        });

    MVulkanDescriptorSetWrite write;
    auto descriptorSet = m_descriptorSets;

    int set = resources[0].m_set;
    std::vector<PassResources> _resources;
    for (auto i = 0; i < resources.size(); i++)
    {
        const auto resource = resources[i];
        ShaderResourceKey key{
            .set = resource.m_set,
            .binding = resource.m_binding
        };
        updateResourceCache(key, resource);

        if (resource.m_set != set)
        {
            //auto descriptorSet = GetDescriptorSets(frameIndex);
            if (descriptorSet.find(set) == descriptorSet.end())
            {
                spdlog::error("error set");
            }

            auto descriptorset = descriptorSet[set];
            write.Update(m_device.GetDevice(), descriptorset.Get(), _resources);
            _resources.clear();
            set = resource.m_set;
        }

        _resources.emplace_back(resource);
    }

    {
        //auto descriptorSet = GetDescriptorSets(frameIndex);
        if (descriptorSet.find(set) == descriptorSet.end())
        {
            spdlog::error("error set");
        }
        auto& descriptorset = descriptorSet[set];
        write.Update(m_device.GetDevice(), descriptorset.Get(), _resources);
        _resources.clear();
    }
}

void ComputePass::CreatePipeline(MVulkanDescriptorSetAllocator& allocator)
{
	MVulkanShaderReflector compReflector(m_shader->GetComputeShader().GetShader());

    ShaderReflectorOut resourceOutComp = compReflector.GenerateShaderReflactorOut2();

    //std::vector<MVulkanDescriptorSetLayoutBinding> bindings = resourceOutComp.GetBindings();
    //m_bindings = resourceOutComp.GetBindings();
    std::vector<std::vector<MVulkanDescriptorSetLayoutBinding>> bindings = resourceOutComp.GetBindings2();
    auto maxSet = bindings.size();

    m_descriptorLayouts.resize(maxSet);
    for (int set = 0; set < maxSet; set++) {
        m_descriptorLayouts[set].Create(m_device.GetDevice(), bindings[set]);
    }

    std::vector<VkDescriptorSetLayout> layouts(maxSet);
    for (int set = 0; set < maxSet; set++) {
        layouts[set] = m_descriptorLayouts[set].Get();
    }

    for (auto set = 0; set < maxSet; set++)
    {
        MVulkanDescriptorSet descriptorSet;
        m_descriptorSets.insert({ set, descriptorSet });
    }

    //for (int i = 0; i < m_descriptorSets.size(); i++) {
    auto& sets = m_descriptorSets;
    for (auto& set : sets) {
        int setIndex = set.first;
        auto& descriptorSet = set.second;
        descriptorSet.Create(m_device.GetDevice(), allocator.Get(), layouts[setIndex]);
    }
    //}

    for (auto set = 0; set < maxSet; set++)
    {
        auto& descriptorSetLayoutBinding = bindings[set];
        for (auto i = 0; i < descriptorSetLayoutBinding.size(); i++)
        {
            auto& binding = descriptorSetLayoutBinding[i];

            if (binding.binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                BufferCreateInfo info;
                info.arrayLength = binding.binding.descriptorCount;
                info.size = binding.size;

                if (binding.name.substr(0, 5) == "type.")
                    binding.name = binding.name.substr(5);

                Singleton<ShaderResourceManager>::instance().AddConstantBuffer(binding.name, info, 1);
            }
        }
    }

    m_pipeline.Create(
        m_device.GetDevice(), 
        m_shader->GetComputeShader().GetShaderModule(),
        layouts,
        m_shader->GetEntryPoint());

    m_shader->Clean();

    for (int set = 0; set < bindings.size(); set++) {
        for (int i = 0; i < bindings[set].size(); i++) {
            auto& binding = bindings[set][i];

            ShaderResourceKey key{
                .set = set,
                .binding = int(binding.binding.binding)
            };

            m_bindings[key] = binding;
        }
    }
}

//void ComputePass::RecreateStorageImages(std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos) {
//    for (auto i = 0; i < m_storageImages.size(); i++) {
//        for (auto j = 0; j < m_storageImages[i].size(); j++) {
//            m_storageImages[i][j]->Clean();
//        }
//    }
//
//    for (auto _binding = 0; _binding < storageImageCreateInfos.size(); _binding++) {
//        m_storageImages[_binding].resize(storageImageCreateInfos[_binding].size());
//        for (auto i = 0; i < m_storageImages[_binding].size(); i++) {
//            m_storageImages[_binding][i] = std::make_shared<MVulkanTexture>();
//
//            ImageCreateInfo imageInfo;
//            ImageViewCreateInfo viewInfo;
//            imageInfo.arrayLength = m_bindings[_binding].binding.descriptorCount;
//            imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
//            imageInfo.width = storageImageCreateInfos[_binding][i].width;
//            imageInfo.height = storageImageCreateInfos[_binding][i].height;
//            imageInfo.format = storageImageCreateInfos[_binding][i].format;
//
//            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//            viewInfo.format = imageInfo.format;
//            viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
//            viewInfo.baseMipLevel = 0;
//            viewInfo.levelCount = 1;
//            viewInfo.baseArrayLayer = 0;
//            viewInfo.layerCount = 1;
//
//            Singleton<MVulkanEngine>::instance().CreateImage(m_storageImages[_binding][i], imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
//        }
//    }
//}

//void ComputePass::UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    MVulkanDescriptorSetWrite write;
//
//    std::vector<std::vector<VkDescriptorImageInfo>>     combinedImageInfos(0);
//    std::vector<std::vector<VkDescriptorImageInfo>>     separateImageInfos(m_separateImageCount);
//    std::vector<std::vector<VkDescriptorImageInfo>>     storageImageInfos(m_storageImageCount);
//    std::vector<VkDescriptorImageInfo>                  separateSamplerInfos(m_separateSamplerCount);
//    std::vector<std::vector<VkDescriptorBufferInfo>>    constantBufferInfos(m_cbvCount);
//    std::vector<std::vector<VkDescriptorBufferInfo>>    storageBufferInfos(m_storageBufferCount);
//
//
//    for (auto binding = 0; binding < m_separateImageCount; binding++) {
//        separateImageInfos[binding].resize(seperateImageViews[binding].size());
//        for (auto j = 0; j < seperateImageViews[binding].size(); j++) {
//            separateImageInfos[binding][j].sampler = nullptr;
//            separateImageInfos[binding][j].imageView = seperateImageViews[binding][j];
//            separateImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        }
//    }
//
//    for (auto binding = 0; binding < m_storageImageCount; binding++) {
//        storageImageInfos[binding].resize(m_storageImages[binding].size());
//        for (auto j = 0; j < m_storageImages[binding].size(); j++) {
//            storageImageInfos[binding][j].sampler = nullptr;
//            storageImageInfos[binding][j].imageView = m_storageImages[binding][j]->GetImageView();
//            storageImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//        }
//    }
//
//    for (auto binding = 0; binding < m_separateSamplerCount; binding++) {
//        separateSamplerInfos[binding].sampler = samplers[binding];
//        separateSamplerInfos[binding].imageView = nullptr;
//        separateSamplerInfos[binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//    }
//
//
//    for (auto binding = 0; binding < m_cbvCount; binding++) {
//        constantBufferInfos[binding].resize(m_constantBuffer[binding].GetArrayLength());
//        for (auto j = 0; j < m_constantBuffer[binding].GetArrayLength(); j++) {
//            constantBufferInfos[binding][j].buffer = m_constantBuffer[binding].GetBuffer();
//            constantBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_constantBuffer[binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
//            constantBufferInfos[binding][j].range = m_constantBuffer[binding].GetBufferSize();
//        }
//    }
//
//    for (auto binding = 0; binding < m_storageBufferCount; binding++) {
//        storageBufferInfos[binding].resize(m_storageBuffer[binding].GetArrayLength());
//        for (auto j = 0; j < m_storageBuffer[binding].GetArrayLength(); j++) {
//            storageBufferInfos[binding][j].buffer = m_storageBuffer[binding].GetBuffer();
//            storageBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_storageBuffer[binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
//            storageBufferInfos[binding][j].range = m_storageBuffer[binding].GetBufferSize();
//        }
//    }
//
//    write.Update(m_device.GetDevice(), m_descriptorSet.Get(), 
//        constantBufferInfos, storageBufferInfos, 
//        combinedImageInfos, separateImageInfos, storageImageInfos, separateSamplerInfos, accelerationStructures);
//}
//
//void ComputePass::UpdateDescriptorSetWrite(std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<std::vector<VkImageView>> storageImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    MVulkanDescriptorSetWrite write;
//
//    std::vector<std::vector<VkDescriptorImageInfo>>     combinedImageInfos(0);
//    std::vector<std::vector<VkDescriptorImageInfo>>     separateImageInfos(m_separateImageCount);
//    std::vector<std::vector<VkDescriptorImageInfo>>     storageImageInfos(m_storageImageCount);
//    std::vector<VkDescriptorImageInfo>                  separateSamplerInfos(m_separateSamplerCount);
//    std::vector<std::vector<VkDescriptorBufferInfo>>    constantBufferInfos(m_cbvCount);
//    std::vector<std::vector<VkDescriptorBufferInfo>>    storageBufferInfos(m_storageBufferCount);
//
//
//    for (auto binding = 0; binding < m_separateImageCount; binding++) {
//        separateImageInfos[binding].resize(seperateImageViews[binding].size());
//        for (auto j = 0; j < seperateImageViews[binding].size(); j++) {
//            separateImageInfos[binding][j].sampler = nullptr;
//            separateImageInfos[binding][j].imageView = seperateImageViews[binding][j];
//            separateImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        }
//    }
//
//    for (auto binding = 0; binding < m_storageImageCount; binding++) {
//        storageImageInfos[binding].resize(storageImageViews[binding].size());
//        for (auto j = 0; j < storageImageViews[binding].size(); j++) {
//            storageImageInfos[binding][j].sampler = nullptr;
//            storageImageInfos[binding][j].imageView = storageImageViews[binding][j];
//            storageImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//        }
//    }
//
//    for (auto binding = 0; binding < m_separateSamplerCount; binding++) {
//        separateSamplerInfos[binding].sampler = samplers[binding];
//        separateSamplerInfos[binding].imageView = nullptr;
//        separateSamplerInfos[binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//    }
//
//
//    for (auto binding = 0; binding < m_cbvCount; binding++) {
//        constantBufferInfos[binding].resize(m_constantBuffer[binding].GetArrayLength());
//        for (auto j = 0; j < m_constantBuffer[binding].GetArrayLength(); j++) {
//            constantBufferInfos[binding][j].buffer = m_constantBuffer[binding].GetBuffer();
//            constantBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_constantBuffer[binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
//            constantBufferInfos[binding][j].range = m_constantBuffer[binding].GetBufferSize();
//        }
//    }
//
//    for (auto binding = 0; binding < m_storageBufferCount; binding++) {
//        storageBufferInfos[binding].resize(m_storageBuffer[binding].GetArrayLength());
//        for (auto j = 0; j < m_storageBuffer[binding].GetArrayLength(); j++) {
//            storageBufferInfos[binding][j].buffer = m_storageBuffer[binding].GetBuffer();
//            storageBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_storageBuffer[binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
//            storageBufferInfos[binding][j].range = m_storageBuffer[binding].GetBufferSize();
//        }
//    }
//
//    write.Update(m_device.GetDevice(), m_descriptorSet.Get(),
//        constantBufferInfos, storageBufferInfos,
//        combinedImageInfos, separateImageInfos, storageImageInfos, separateSamplerInfos, accelerationStructures);
//}

//void ComputePass::UpdateDescriptorSetWrite(std::vector<StorageBuffer> storageBuffers, std::vector<std::vector<VkImageView>> seperateImageViews, std::vector<std::vector<VkImageView>> storageImageViews, std::vector<VkSampler> samplers, std::vector<VkAccelerationStructureKHR> accelerationStructures)
//{
//    MVulkanDescriptorSetWrite write;
//
//    std::vector<std::vector<VkDescriptorImageInfo>>     combinedImageInfos(0);
//    std::vector<std::vector<VkDescriptorImageInfo>>     separateImageInfos(m_separateImageCount);
//    std::vector<std::vector<VkDescriptorImageInfo>>     storageImageInfos(m_storageImageCount);
//    std::vector<VkDescriptorImageInfo>                  separateSamplerInfos(m_separateSamplerCount);
//    std::vector<std::vector<VkDescriptorBufferInfo>>    constantBufferInfos(m_cbvCount);
//    std::vector<std::vector<VkDescriptorBufferInfo>>    storageBufferInfos(m_storageBufferCount);
//
//
//    for (auto binding = 0; binding < m_separateImageCount; binding++) {
//        separateImageInfos[binding].resize(seperateImageViews[binding].size());
//        for (auto j = 0; j < seperateImageViews[binding].size(); j++) {
//            separateImageInfos[binding][j].sampler = nullptr;
//            separateImageInfos[binding][j].imageView = seperateImageViews[binding][j];
//            separateImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        }
//    }
//
//    for (auto binding = 0; binding < m_storageImageCount; binding++) {
//        storageImageInfos[binding].resize(storageImageViews[binding].size());
//        for (auto j = 0; j < storageImageViews[binding].size(); j++) {
//            storageImageInfos[binding][j].sampler = nullptr;
//            storageImageInfos[binding][j].imageView = storageImageViews[binding][j];
//            storageImageInfos[binding][j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//        }
//    }
//
//    for (auto binding = 0; binding < m_separateSamplerCount; binding++) {
//        separateSamplerInfos[binding].sampler = samplers[binding];
//        separateSamplerInfos[binding].imageView = nullptr;
//        separateSamplerInfos[binding].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//    }
//
//
//    for (auto binding = 0; binding < m_cbvCount; binding++) {
//        constantBufferInfos[binding].resize(m_constantBuffer[binding].GetArrayLength());
//        for (auto j = 0; j < m_constantBuffer[binding].GetArrayLength(); j++) {
//            constantBufferInfos[binding][j].buffer = m_constantBuffer[binding].GetBuffer();
//            constantBufferInfos[binding][j].offset = j * CalculateAlignedSize(m_constantBuffer[binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
//            constantBufferInfos[binding][j].range = m_constantBuffer[binding].GetBufferSize();
//        }
//    }
//
//    for (auto binding = 0; binding < storageBuffers.size(); binding++) {
//        storageBufferInfos[binding].resize(storageBuffers[binding].GetArrayLength());
//        for (auto j = 0; j < storageBuffers[binding].GetArrayLength(); j++) {
//            storageBufferInfos[binding][j].buffer = storageBuffers[binding].GetBuffer();
//            storageBufferInfos[binding][j].offset = j * CalculateAlignedSize(storageBuffers[binding].GetBufferSize(), Singleton<MVulkanEngine>::instance().GetUniformBufferOffsetAlignment());
//            storageBufferInfos[binding][j].range = storageBuffers[binding].GetBufferSize();
//        }
//    }
//
//    write.Update(m_device.GetDevice(), m_descriptorSet.Get(),
//        constantBufferInfos, storageBufferInfos,
//        combinedImageInfos, separateImageInfos, storageImageInfos, separateSamplerInfos, accelerationStructures);
//}

MVulkanDescriptorSetLayouts ComputePass::GetDescriptorSetLayout(int set) const
{
    return m_descriptorLayouts[set];
}

std::unordered_map<int, MVulkanDescriptorSet> ComputePass::GetDescriptorSets() const
{
    return m_descriptorSets;
}

StorageBuffer ComputePass::GetStorageBufferByBinding(uint32_t binding)
{
    return m_storageBuffer[binding - m_cbvCount];
}

VkImageView ComputePass::GetStorageImageViewByBinding(uint32_t binding, uint32_t arrayIndex) {
    return m_storageImages[binding][arrayIndex]->GetImageView();
}

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

void ComputePass::PrepareResourcesForShaderRead()
{
    TextureState state;
    state.m_stage = ShaderStageFlagBits::COMPUTE;

    for (auto& it : m_cachedResources) {
        auto resource = it.second;
        auto key = it.first;
        if (resource.m_textures.size() != 0) {
            for (auto& texture : resource.m_textures) {
                auto binding = m_bindings[key];
                auto shaderStage = binding.binding.stageFlags;
                state.m_stage = VkShaderStage2ShaderStage(shaderStage);
                state.m_state = VkDescriptorType2ETextureState(binding.binding.descriptorType);
                texture->TransferTextureState(-1, state);
            }
        }
    }
}

void ComputePass::PrepareResourcesForShaderRead(MVulkanCommandList commandList)
{
    TextureState state;
    state.m_stage = ShaderStageFlagBits::COMPUTE;

    for (auto& it : m_cachedResources) {
        auto resource = it.second;
        auto key = it.first;
        if (resource.m_textures.size() != 0) {
            for (auto& texture : resource.m_textures) {
                auto binding = m_bindings[key];
                auto shaderStage = binding.binding.stageFlags;
                state.m_stage = VkShaderStage2ShaderStage(shaderStage);
                state.m_state = VkDescriptorType2ETextureState(binding.binding.descriptorType);
                texture->TransferTextureState(commandList, state);
            }
        }
    }
}

void ComputePass::setShader(std::shared_ptr<ComputeShaderModule> shader)
{
    m_shader = shader;
    m_shader->Create(m_device.GetDevice());
}

void ComputePass::updateResourceCache(ShaderResourceKey key, const PassResources& resources)
{
    m_cachedResources[key] = resources;
}
