#include "Managers/TextureManager.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

bool TextureManager::ExistTexture(const std::string& name)
{
    return m_textureMap.find(name)!= m_textureMap.end();
}

void TextureManager::Clean()
{
    for (auto texture : m_textureMap) {
        if (texture.first.size() != 0 && texture.first != "")
            texture.second->Clean();
    }

    m_textureMapIdx.clear();
    m_textureMap.clear();
}

std::vector<std::shared_ptr<MVulkanTexture>> TextureManager::GenerateTextureVector()
{
    m_textureMapIdx.clear();
    std::vector<std::shared_ptr<MVulkanTexture>> textures(m_textureMap.size());

    int idx = 0;
    for (auto item : m_textureMap) {
        auto name = item.first;

        m_textureMapIdx[name] = idx;
        textures[idx] = item.second;

        idx++;
    }

    return textures;
}

std::vector<std::shared_ptr<MVulkanTexture>> TextureManager::GenerateUnmipedTextureVector()
{
    std::vector<std::shared_ptr<MVulkanTexture>> _textures(0);
    
    for (auto item : m_textureMap) {
        if (!item.second->MipMapGenerated()) {
            _textures.push_back(item.second);
        }
    }

    return _textures;
}


std::vector<VkImageView> TextureManager::GenerateTextureViews() {
    std::vector<VkImageView> views;

    auto wholeTextures = GenerateTextureVector();
    auto wholeTextureSize = wholeTextures.size();
    
    if (wholeTextureSize == 0) {
        views.resize(1);
        views[0] = Singleton<MVulkanEngine>::instance().GetPlaceHolderTexture()->GetImageView();
    }
    else {
        views.resize(wholeTextureSize);
        for (auto j = 0; j < wholeTextureSize; j++) {
            views[j] = wholeTextures[j]->GetImageView();
        }
    }

    return views;
}

std::vector<std::shared_ptr<MVulkanTexture>> TextureManager::GenerateTextures() {
    std::vector<std::shared_ptr<MVulkanTexture>> textures;

    auto wholeTextures = GenerateTextureVector();
    auto wholeTextureSize = wholeTextures.size();

    if (wholeTextureSize == 0) {
        textures.resize(1);
        textures[0] = Singleton<MVulkanEngine>::instance().GetPlaceHolderTexture();
    }
    else {
        textures.resize(wholeTextureSize);
        for (auto j = 0; j < wholeTextureSize; j++) {
            textures[j] = wholeTextures[j];
        }
    }

    return textures;
}
