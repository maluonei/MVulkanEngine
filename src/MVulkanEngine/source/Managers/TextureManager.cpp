#include "Managers/TextureManager.hpp"

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
