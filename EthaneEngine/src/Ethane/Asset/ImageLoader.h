//
//  ImageLoader.hpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2023/5/22.
//
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Ethane {

struct ImageResource {
    uint32_t Width;
    uint32_t Height;
    uint32_t OriginalChannels;
    void* Data;
    uint32_t DataSize;
};

class ImageLoader
{
public:
    // Decodes to RGBA8 (4 channels whatever the file has), top row first
    bool LoadImage(const std::string& filepath, ImageResource& out_resource);
    // Same, from an encoded image (png, jpg, ...) in memory, e.g. a texture embedded in a model file
    bool LoadImageFromMemory(const void* encoded, size_t size, ImageResource& out_resource);
    
    void UnloadImage(ImageResource& resource);
};

// Decoded pixels that own their memory: RGBA8, top row first. Decoding needs no GPU, so it can run on
// any thread; GfxBackendAPI::CreateTexture2D(image) uploads it.
struct DecodedImage
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    std::vector<uint8_t> Pixels; // Width * Height * 4 bytes
};

// A file (png, jpg, ...) or encoded bytes in memory; nullopt (and a warning) if it can't be decoded
std::optional<DecodedImage> DecodeImage(const std::string& filepath);
std::optional<DecodedImage> DecodeImage(const void* encoded, size_t size);
}
