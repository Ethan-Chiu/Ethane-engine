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

        m_Image = vk::raii::Image(vkDevice, vk::ImageCreateInfo{
            .imageType     = vk::ImageType::e2D,
            .format        = m_Format,
            .extent        = { spec.Width, spec.Height, 1 },
            .mipLevels     = 1,
            .arrayLayers   = 1,
            .samples       = vk::SampleCountFlagBits::e1,
            .tiling        = vk::ImageTiling::eOptimal,
            .usage         = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
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
            .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }});

        // samplerAnisotropy is required by device selection and enabled with the supported features
        const bool anisotropy = physicalDevice.GetEnabledFeatures().get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy;
        const float maxAnisotropy = physicalDevice.GetInfo().Properties.get<vk::PhysicalDeviceProperties2>().properties.limits.maxSamplerAnisotropy;
        const vk::Filter filter = VulkanFilter(spec.Filter);
        const vk::SamplerAddressMode wrap = VulkanWrap(spec.Wrap);
        m_Sampler = vk::raii::Sampler(vkDevice, vk::SamplerCreateInfo{
            .magFilter        = filter,
            .minFilter        = filter,
            .mipmapMode       = vk::SamplerMipmapMode::eLinear,
            .addressModeU     = wrap,
            .addressModeV     = wrap,
            .addressModeW     = wrap,
            .mipLodBias       = 0.0f,
            .anisotropyEnable = anisotropy ? vk::True : vk::False,
            .maxAnisotropy    = anisotropy ? std::min(16.0f, maxAnisotropy) : 1.0f,
            .compareEnable    = vk::False,
            .minLod           = 0.0f,
            .maxLod           = 0.0f, // single mip level
            .borderColor      = vk::BorderColor::eIntOpaqueBlack});
    }

    void VulkanTexture2D::Upload(const void* pixels, uint32_t size)
    {
        VulkanBuffer staging{ m_Device };
        staging.CreateVulkanBuffer(size, vk::BufferUsageFlagBits::eTransferSrc,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, true);
        staging.SetData(pixels, 0, size, 0);

        const vk::ImageSubresourceRange range{ .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 };

        VulkanCommandBuffer cmd{ m_Device };
        cmd.AllocateAndBeginSingleUse(QueueFamilyTypes::Graphics);
        const vk::raii::CommandBuffer& cb = cmd.Get();

        // undefined -> transfer dst: nothing to wait for, the copy waits for the transition
        cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
            vk::ImageMemoryBarrier{
                .srcAccessMask       = {},
                .dstAccessMask       = vk::AccessFlagBits::eTransferWrite,
                .oldLayout           = vk::ImageLayout::eUndefined,
                .newLayout           = vk::ImageLayout::eTransferDstOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *m_Image,
                .subresourceRange    = range});

        cb.copyBufferToImage(staging.GetHandle(), *m_Image, vk::ImageLayout::eTransferDstOptimal, vk::BufferImageCopy{
            .bufferOffset      = 0,
            .bufferRowLength   = 0, // tightly packed
            .bufferImageHeight = 0,
            .imageSubresource  = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
            .imageOffset       = { 0, 0, 0 },
            .imageExtent       = { m_Specification.Width, m_Specification.Height, 1 }});

        // transfer dst -> shader read: any shader stage that samples it waits for the copy
        cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                           vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader,
                           {}, {}, {},
            vk::ImageMemoryBarrier{
                .srcAccessMask       = vk::AccessFlagBits::eTransferWrite,
                .dstAccessMask       = vk::AccessFlagBits::eShaderRead,
                .oldLayout           = vk::ImageLayout::eTransferDstOptimal,
                .newLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *m_Image,
                .subresourceRange    = range});

        cmd.EndSingleUse(QueueFamilyTypes::Graphics); // waits, so the staging buffer can go at the end of this scope
    }

    vk::DescriptorImageInfo VulkanTexture2D::GetDescriptorInfo() const
    {
        return vk::DescriptorImageInfo{
            .sampler     = *m_Sampler,
            .imageView   = *m_ImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
    }

}
