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

    namespace {
        RefCountPtr<ITexture2D> CreateFromImage(GfxBackendAPI& backend, ImageLoader& loader, ImageResource& image, TextureFilter filter, TextureWrap wrap)
        {
            const TextureSpecification spec{
                .Width  = image.Width,
                .Height = image.Height,
                .Format = ImageFormat::RGBA, // ImageLoader always expands to 4 channels
                .Filter = filter,
                .Wrap   = wrap,
            };
            RefCountPtr<ITexture2D> texture = backend.CreateTexture2D(spec, image.Data); // copies the pixels
            loader.UnloadImage(image);
            return texture;
        }
    }

    RefCountPtr<ITexture2D> GfxBackendAPI::LoadTexture2D(const std::string& filepath, TextureFilter filter, TextureWrap wrap)
    {
        ImageLoader loader;
        ImageResource image{};
        if (!loader.LoadImage(filepath, image)) {
            ETH_CORE_ERROR("Failed to load texture {0}", filepath);
            return {};
        }
        return CreateFromImage(*this, loader, image, filter, wrap);
    }

    RefCountPtr<ITexture2D> GfxBackendAPI::LoadTexture2DFromMemory(const void* encoded, size_t size, TextureFilter filter, TextureWrap wrap)
    {
        ImageLoader loader;
        ImageResource image{};
        if (!loader.LoadImageFromMemory(encoded, size, image)) {
            ETH_CORE_ERROR("Failed to decode an in-memory texture ({0} bytes)", size);
            return {};
        }
        return CreateFromImage(*this, loader, image, filter, wrap);
    }

    const RefCountPtr<ITexture2D>& GfxBackendAPI::GetDefaultTexture(DefaultTexture which)
    {
        RefCountPtr<ITexture2D>& texture = m_DefaultTextures[static_cast<size_t>(which)];
        if (!texture) {
            // flat normal: tangent-space (0, 0, 1) encoded as (n + 1) / 2
            const uint8_t pixel[4] = { which == DefaultTexture::FlatNormal ? uint8_t(128) : uint8_t(255),
                                       which == DefaultTexture::FlatNormal ? uint8_t(128) : uint8_t(255), 255, 255 };
            texture = CreateTexture2D(TextureSpecification{ .Width = 1, .Height = 1, .Format = ImageFormat::RGBA,
                                                            .Filter = TextureFilter::Nearest, .Wrap = TextureWrap::Repeat }, pixel);
        }
        return texture;
    }

    void GfxBackendAPI::ReleaseDefaultResources()
    {
        for (RefCountPtr<ITexture2D>& texture : m_DefaultTextures) {
            texture.reset();
        }
    }

}
