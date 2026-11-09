//
//  ImageLoader.hpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2023/5/22.
//
#pragma once

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
}
