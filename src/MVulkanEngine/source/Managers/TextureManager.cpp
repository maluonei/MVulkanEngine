#include "Managers/TextureManager.hpp"

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
