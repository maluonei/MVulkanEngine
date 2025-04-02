#pragma once
#ifndef TEXTUREMANAGER_HPP
#define TEXTUREMANAGER_HPP

#include "Singleton.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"

class TextureManager : public Singleton<TextureManager> {
public:
	inline void Put(const std::string& name, std::shared_ptr<MVulkanTexture> texture) {
		m_textureMap[name] = texture;
	}

	inline std::shared_ptr<MVulkanTexture> Get(const std::string& name){
		return m_textureMap[name];
	}

	inline int GetTextureId(const std::string& name) {
		return m_textureMapIdx[name];
	}

	bool ExistTexture(const std::string& name);

	void Clean();

	std::vector<std::shared_ptr<MVulkanTexture>> GenerateTextureVector();

	std::vector<VkImageView> GenerateTextureViews();

protected:
	virtual void InitSingleton() {
		
	}

private:
	std::unordered_map<std::string, std::shared_ptr<MVulkanTexture>> m_textureMap;
	std::unordered_map<std::string, int> m_textureMapIdx;
};


#endif