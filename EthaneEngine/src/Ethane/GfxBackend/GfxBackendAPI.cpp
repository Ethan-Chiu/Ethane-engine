//
//  GfxBackendAPI.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//

#include "GfxBackendAPI.h"
#include "Vulkan/VulkanGfxBackendAPI.h"
#include "Ethane/Asset/ImageLoader.h"

namespace Ethane {

GfxBackendAPI::API GfxBackendAPI::s_API = GfxBackendAPI::API::Vulkan;

    Scope<GfxBackendAPI> GfxBackendAPI::Create()
    {
        switch (s_API)
        {
        case API::None:
            ETH_CORE_ASSERT(false, "rendererAPI::None is currently not supported!");
            return nullptr;
        case API::Vulkan:
            return CreateScope<VulkanGfxBackendAPI>();
        }

        ETH_CORE_ASSERT(false, "Unknown RendererAPI");
        return nullptr;
    }

    RefCountPtr<ITexture2D> GfxBackendAPI::LoadTexture2D(const std::string& filepath, TextureFilter filter, TextureWrap wrap)
    {
        ImageLoader loader;
        ImageResource image{};
        if (!loader.LoadImage(filepath, image)) {
            ETH_CORE_ERROR("Failed to load texture {0}", filepath);
            return {};
        }

        const TextureSpecification spec{
            .Width  = image.Width,
            .Height = image.Height,
            .Format = ImageFormat::RGBA, // ImageLoader always expands to 4 channels
            .Filter = filter,
            .Wrap   = wrap,
        };
        RefCountPtr<ITexture2D> texture = CreateTexture2D(spec, image.Data); // copies the pixels
        loader.UnloadImage(image);
        return texture;
    }

}
