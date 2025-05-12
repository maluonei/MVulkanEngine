#pragma once
#ifndef MIMAGE_H
#define MIMAGE_H

#include <stb_image.h>
#include <filesystem>
#include <vulkan/vulkan_core.h>
#include <assert.h>
//#include <DirectXTex.h>
#include <gli.hpp>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

template <typename T>
class MImage {
public:
    bool Load(fs::path imagePath)
    {
        auto imageExtension = imagePath.extension().string();
        assert(fs::exists(imagePath), "imagePath not exist");

        if (imageExtension == ".jpg" || imageExtension == ".png" || imageExtension == ".jpeg") {
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
        else if(imageExtension == ".dds") {
            gli::texture texture = gli::load(imagePath.string().c_str());
            if (texture.empty()) {
                spdlog::error("Failed to load .dds file {0}", imagePath.string().c_str());
                //std::cerr << "Failed to load .dds file" << std::endl;
                return false;
            }

            //std::cout << "Loaded texture: "
            //    << texture.extent().x << "x" << texture.extent().y
            //    << ", layers: " << texture.layers()
            //    << ", mip levels: " << texture.levels() << std::endl;

            texWidth = texture.extent().x;
            texHeight = texture.extent().y;
            mipLevels = texture.levels();

            gli::format fmt = texture.format();
            //gli::format_desc desc = gli::format_desc(fmt);

            std::size_t mipLevel = 0;
            //gli::image img = texture[mipLevel];

            std::size_t size = texture.size(mipLevel);
            texChannels = size / (texWidth * texHeight);

            std::size_t pixelOrBlockSize = gli::block_size(fmt);
            std::size_t numChannels = gli::component_count(fmt);

            if (texChannels == 1) {
                format = VkFormat::VK_FORMAT_R8_SRGB;
            }
            else if (texChannels == 2) {
                format = VkFormat::VK_FORMAT_R8G8_SRGB;
            }
            else if (texChannels == 3) {
                format = VkFormat::VK_FORMAT_R8G8B8_SRGB;
            }
            else {
                format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
            }

            //format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
            //texChannels = texture.extent().depth;

            // 获取第一个 mip level 数据
            data = static_cast<T*>(texture.data(0, 0, 0));  // layer, face, level

            return true;
        }
        else {
            spdlog::error("unknow image extension: {0}, {1}", imagePath.string().c_str(), imageExtension.c_str());
        }
    }

    ~MImage() {
        if(data !=nullptr)
            stbi_image_free(data);
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
