//
//  ImageLoader.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2023/5/22.
//

#include "ethpch.h"

#include "ImageLoader.h"
#include "stb_image.h"

namespace Ethane {

    namespace {
        bool Fill(stbi_uc* data, int width, int height, int channels, ImageResource& out_resource)
        {
            if (!data) {
                // callers report the failure (a missing texture isn't fatal)
                ETH_CORE_WARN("stb_image: {0}", stbi_failure_reason());
                return false;
            }
            out_resource.Width = width;
            out_resource.Height = height;
            out_resource.OriginalChannels = channels;
            out_resource.DataSize = width * height * 4;
            out_resource.Data = data;
            ETH_CORE_TRACE("w: {0}, h: {1}, ch: {2}", out_resource.Width, out_resource.Height, out_resource.OriginalChannels);
            return true;
        }
    }

    bool ImageLoader::LoadImage(const std::string& filepath, ImageResource& out_resource)
    {
        int width = 0, height = 0, channels = 0;
        stbi_uc* data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        return Fill(data, width, height, channels, out_resource);
    }

    bool ImageLoader::LoadImageFromMemory(const void* encoded, size_t size, ImageResource& out_resource)
    {
        int width = 0, height = 0, channels = 0;
        stbi_uc* data = stbi_load_from_memory(static_cast<const stbi_uc*>(encoded), static_cast<int>(size), &width, &height, &channels, STBI_rgb_alpha);
        return Fill(data, width, height, channels, out_resource);
    }

    void ImageLoader::UnloadImage(ImageResource& resource)
    {
        stbi_image_free(resource.Data);
    }

    namespace {
        std::optional<DecodedImage> Own(ImageLoader& loader, ImageResource& image)
        {
            DecodedImage decoded{ .Width = image.Width, .Height = image.Height };
            const uint8_t* pixels = static_cast<const uint8_t*>(image.Data);
            decoded.Pixels.assign(pixels, pixels + size_t(image.Width) * image.Height * 4);
            loader.UnloadImage(image);
            return decoded;
        }
    }

    std::optional<DecodedImage> DecodeImage(const std::string& filepath)
    {
        ImageLoader loader;
        ImageResource image{};
        if (!loader.LoadImage(filepath, image)) {
            ETH_CORE_WARN("Couldn't decode image {0}", filepath);
            return std::nullopt;
        }
        return Own(loader, image);
    }

    std::optional<DecodedImage> DecodeImage(const void* encoded, size_t size)
    {
        ImageLoader loader;
        ImageResource image{};
        if (!loader.LoadImageFromMemory(encoded, size, image)) {
            ETH_CORE_WARN("Couldn't decode an in-memory image ({0} bytes)", size);
            return std::nullopt;
        }
        return Own(loader, image);
    }

}
