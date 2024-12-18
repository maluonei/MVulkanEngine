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

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        data = static_cast<T*>(pixels);

        return true;
    }

	inline T* GetData() { return data; }

	inline const int Width() const { return texWidth; }
	inline const int Height() const { return texHeight; }
	inline const int Channels() const { return texChannels; }
    inline const uint32_t MipLevels() const { return mipLevels; }
	inline const VkFormat Format() const { return format; }

    static MImage<T> GetDefault() {
        MImage<T> image;
        image.texWidth = 1;
        image.texHeight = 1;
        image.texChannels = 4;
        image.format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
        image.mipLevels = 1;
        image.data = new T[4];

        return image;
    }
private:
    uint32_t mipLevels = 1;
	int texWidth, texHeight, texChannels;
	VkFormat format;

	T* data = nullptr;
};

#endif
