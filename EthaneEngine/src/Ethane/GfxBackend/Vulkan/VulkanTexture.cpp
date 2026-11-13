//
//  VulkanTexture.cpp
//  EthaneEngine
//
#include "ethpch.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImageUtils.h"

namespace Ethane {

    namespace {
        uint32_t BytesPerTexel(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGBA:
            case ImageFormat::BGRA:
            case ImageFormat::RED32F:
            case ImageFormat::RG16F:   return 4;
            case ImageFormat::RG32F:
            case ImageFormat::RGBA16F: return 8;
            case ImageFormat::RGBA32F: return 16;
            default: break;
            }
            // 3-channel formats are rarely sampleable on GPUs: expand to 4 channels on load instead
            ETH_CORE_ASSERT(false, "Unsupported texture format");
            return 0;
        }

        vk::Filter VulkanFilter(TextureFilter filter)
        {
            return filter == TextureFilter::Nearest ? vk::Filter::eNearest : vk::Filter::eLinear;
        }

        vk::SamplerAddressMode VulkanWrap(TextureWrap wrap)
        {
            switch (wrap)
            {
            case TextureWrap::ClampToEdge:    return vk::SamplerAddressMode::eClampToEdge;
            case TextureWrap::MirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
            case TextureWrap::Repeat:         break;
            }
            return vk::SamplerAddressMode::eRepeat;
        }
    }

    VulkanTexture2D::VulkanTexture2D(const VulkanDevice* device, const TextureSpecification& spec, const void* pixels)
        : m_Device(device), m_Specification(spec)
    {
        ETH_CORE_ASSERT(spec.Width > 0 && spec.Height > 0, "Texture needs a non-zero size");
        ETH_CORE_ASSERT(pixels, "Texture needs pixel data");

        const vk::raii::Device& vkDevice = m_Device->Get();
        const VulkanPhysicalDevice& physicalDevice = m_Device->GetPhysicalDevice();

        m_Format = ImageUtils::VulkanImageFormat(spec.Format, physicalDevice);
        if (!physicalDevice.SupportsFormat(m_Format, vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eTransferDst)) {
            ETH_CORE_ERROR("Texture: {0} can't be sampled on this device", vk::to_string(m_Format));
            ETH_CORE_ASSERT(false, "Unsupported texture format");
        }

        if (spec.GenerateMips) {
            // mips are made by linear blits from the level above
            using Feature = vk::FormatFeatureFlagBits;
            if (physicalDevice.SupportsFormat(m_Format, Feature::eBlitSrc | Feature::eBlitDst | Feature::eSampledImageFilterLinear)) {
                m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(spec.Width, spec.Height)))) + 1;
            } else {
                ETH_CORE_WARN("Texture: {0} can't be blitted with linear filtering, no mipmaps", vk::to_string(m_Format));
            }
        }

        m_Image = vk::raii::Image(vkDevice, vk::ImageCreateInfo{
            .imageType     = vk::ImageType::e2D,
            .format        = m_Format,
            .extent        = { spec.Width, spec.Height, 1 },
            .mipLevels     = m_MipLevels,
            .arrayLayers   = 1,
            .samples       = vk::SampleCountFlagBits::e1,
            .tiling        = vk::ImageTiling::eOptimal,
            // transfer src: each mip level is blitted from the previous one
            .usage         = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            .sharingMode   = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined});

        const vk::MemoryRequirements requirements = m_Image.getMemoryRequirements();
        m_Memory = vk::raii::DeviceMemory(vkDevice, vk::MemoryAllocateInfo{
            .allocationSize  = requirements.size,
            .memoryTypeIndex = m_Device->FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)});
        m_Image.bindMemory(*m_Memory, 0);

        Upload(pixels, spec.Width * spec.Height * BytesPerTexel(spec.Format));

        m_ImageView = vk::raii::ImageView(vkDevice, vk::ImageViewCreateInfo{
            .image            = *m_Image,
            .viewType         = vk::ImageViewType::e2D,
            .format           = m_Format,
            .components       = {},
            .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = m_MipLevels, .baseArrayLayer = 0, .layerCount = 1 }});

        // samplerAnisotropy is required by device selection and enabled with the supported features
        const bool anisotropy = physicalDevice.GetEnabledFeatures().get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy;
        const float maxAnisotropy = physicalDevice.GetInfo().Properties.get<vk::PhysicalDeviceProperties2>().properties.limits.maxSamplerAnisotropy;
        const vk::Filter filter = VulkanFilter(spec.Filter);
        const vk::SamplerAddressMode wrap = VulkanWrap(spec.Wrap);
        m_Sampler = vk::raii::Sampler(vkDevice, vk::SamplerCreateInfo{
            .magFilter        = filter,
            .minFilter        = filter,
            .mipmapMode       = spec.Filter == TextureFilter::Nearest ? vk::SamplerMipmapMode::eNearest : vk::SamplerMipmapMode::eLinear,
            .addressModeU     = wrap,
            .addressModeV     = wrap,
            .addressModeW     = wrap,
            .mipLodBias       = 0.0f,
            .anisotropyEnable = anisotropy ? vk::True : vk::False,
            .maxAnisotropy    = anisotropy ? std::min(16.0f, maxAnisotropy) : 1.0f,
            .compareEnable    = vk::False,
            .minLod           = 0.0f,
            .maxLod           = static_cast<float>(m_MipLevels),
            .borderColor      = vk::BorderColor::eIntOpaqueBlack});
    }

    void VulkanTexture2D::Upload(const void* pixels, uint32_t size)
    {
        VulkanBuffer staging{ m_Device };
        staging.CreateVulkanBuffer(size, vk::BufferUsageFlagBits::eTransferSrc,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, true);
        staging.SetData(pixels, 0, size, 0);

        using Stage = vk::PipelineStageFlagBits;
        using Access = vk::AccessFlagBits;
        using Layout = vk::ImageLayout;
        auto barrier = [&](const vk::raii::CommandBuffer& cb, uint32_t baseMip, uint32_t mipCount,
                           Layout oldLayout, Layout newLayout, vk::AccessFlags srcAccess, vk::AccessFlags dstAccess,
                           vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) {
            cb.pipelineBarrier(srcStage, dstStage, {}, {}, {}, vk::ImageMemoryBarrier{
                .srcAccessMask       = srcAccess,
                .dstAccessMask       = dstAccess,
                .oldLayout           = oldLayout,
                .newLayout           = newLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *m_Image,
                .subresourceRange    = { .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = baseMip, .levelCount = mipCount, .baseArrayLayer = 0, .layerCount = 1 }});
        };
        // any shader stage that samples the texture waits for its last write
        const vk::PipelineStageFlags shaderStages = Stage::eVertexShader | Stage::eFragmentShader | Stage::eComputeShader;

        VulkanCommandBuffer cmd{ m_Device };
        cmd.AllocateAndBeginSingleUse(QueueFamilyTypes::Graphics);
        const vk::raii::CommandBuffer& cb = cmd.Get();

        // every level: undefined -> transfer dst (nothing to wait for)
        barrier(cb, 0, m_MipLevels, Layout::eUndefined, Layout::eTransferDstOptimal, {}, Access::eTransferWrite, Stage::eTopOfPipe, Stage::eTransfer);

        cb.copyBufferToImage(staging.GetHandle(), *m_Image, Layout::eTransferDstOptimal, vk::BufferImageCopy{
            .bufferOffset      = 0,
            .bufferRowLength   = 0, // tightly packed
            .bufferImageHeight = 0,
            .imageSubresource  = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
            .imageOffset       = { 0, 0, 0 },
            .imageExtent       = { m_Specification.Width, m_Specification.Height, 1 }});

        // Level i is made from level i - 1, which is then done and goes to shader-read
        int32_t width = static_cast<int32_t>(m_Specification.Width);
        int32_t height = static_cast<int32_t>(m_Specification.Height);
        for (uint32_t level = 1; level < m_MipLevels; level++)
        {
            barrier(cb, level - 1, 1, Layout::eTransferDstOptimal, Layout::eTransferSrcOptimal,
                    Access::eTransferWrite, Access::eTransferRead, Stage::eTransfer, Stage::eTransfer);

            const int32_t nextWidth = std::max(width / 2, 1);
            const int32_t nextHeight = std::max(height / 2, 1);
            cb.blitImage(*m_Image, Layout::eTransferSrcOptimal, *m_Image, Layout::eTransferDstOptimal, vk::ImageBlit{
                .srcSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = level - 1, .baseArrayLayer = 0, .layerCount = 1 },
                .srcOffsets     = std::array<vk::Offset3D, 2>{ vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ width, height, 1 } },
                .dstSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = level, .baseArrayLayer = 0, .layerCount = 1 },
                .dstOffsets     = std::array<vk::Offset3D, 2>{ vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ nextWidth, nextHeight, 1 } }},
                vk::Filter::eLinear);

            barrier(cb, level - 1, 1, Layout::eTransferSrcOptimal, Layout::eShaderReadOnlyOptimal,
                    Access::eTransferRead, Access::eShaderRead, Stage::eTransfer, shaderStages);
            width = nextWidth;
            height = nextHeight;
        }

        // the last level was only written
        barrier(cb, m_MipLevels - 1, 1, Layout::eTransferDstOptimal, Layout::eShaderReadOnlyOptimal,
                Access::eTransferWrite, Access::eShaderRead, Stage::eTransfer, shaderStages);

        // No waiting: frames that sample the texture are submitted later. The staging buffer is retired at the
        // end of this scope, after the command buffer, so it's destroyed only once the upload has finished.
        cmd.EndSingleUseAsync(QueueFamilyTypes::Graphics);
    }

    vk::DescriptorImageInfo VulkanTexture2D::GetDescriptorInfo() const
    {
        return vk::DescriptorImageInfo{
            .sampler     = *m_Sampler,
            .imageView   = *m_ImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
    }

}
