#pragma once
#ifndef MIMAGE_H
#define MIMAGE_H

#include <stb_image.h>
#include <filesystem>
#include <vulkan/vulkan_core.h>

namespace fs = std::filesystem;

template <typename T>
class MImage {
public:
    bool Load(fs::path imagePath)
    {
        //int texWidth, texHeight, texChannels;
        //const char* path = imagePath.string().c_str();
        stbi_uc* pixels = stbi_load(imagePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        //VkDeviceSize imageSize = texWidth * texHeight * 4;

        format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
        //if (texChannels == 4) {
        //    format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
        //}
        //else if (texChannels == 3) {
        //    format = VkFormat::VK_FORMAT_R8G8B8_SRGB;
        //}
        //else if (texChannels == 2) {
        //    format = VkFormat::VK_FORMAT_R8G8_SRGB;
        //}
        //else if (texChannels == 1) {
        //    format = VkFormat::VK_FORMAT_R8_SRGB;
        //}

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        //stbi_image_free(pixels);

        data = static_cast<T*>(pixels);

        return true;
    }

	inline T* GetData() { return data; }

	inline int Width() const { return texWidth; }
	inline int Height() const { return texHeight; }
	inline int Channels() const { return texChannels; }
	inline VkFormat Format() const { return format; }
private:
	int texWidth, texHeight, texChannels;
	VkFormat format;

	T* data = nullptr;
};

#endif
