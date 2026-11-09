//
//  VulkanTexture.h
//  EthaneEngine
//
#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"

namespace Ethane {

    // Device-local sampled image with its view and sampler, ready in eShaderReadOnlyOptimal after construction
    class VulkanTexture2D : public ITexture2D
    {
    public:
        // `pixels`: spec.Width * spec.Height tightly packed texels of spec.Format, uploaded through a staging buffer
        VulkanTexture2D(const VulkanDevice* device, const TextureSpecification& spec, const void* pixels);
        virtual ~VulkanTexture2D() = default;

        virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }
        uint32_t GetMipLevels() const { return m_MipLevels; }

        vk::DescriptorImageInfo GetDescriptorInfo() const;

    private:
        // Copies the pixels to mip 0, blits the rest of the chain, leaves every level in eShaderReadOnlyOptimal
        void Upload(const void* pixels, uint32_t size);

    private:
        const VulkanDevice* m_Device = nullptr;
        VulkanLiveResource m_LiveResource;
        TextureSpecification m_Specification;
        vk::Format m_Format = vk::Format::eUndefined;
        uint32_t m_MipLevels = 1;

        // NOTE: declaration order matters, members are destroyed in reverse: sampler -> view -> image -> memory
        vk::raii::DeviceMemory m_Memory = nullptr;
        vk::raii::Image m_Image = nullptr;
        vk::raii::ImageView m_ImageView = nullptr;
        vk::raii::Sampler m_Sampler = nullptr;
    };

}
