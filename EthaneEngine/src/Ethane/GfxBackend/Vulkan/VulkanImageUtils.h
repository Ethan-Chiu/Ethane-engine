//
//  VulkanImageUtils.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2023/5/16.
//

#pragma once

#include "ethpch.h"
#include "Vulkan.h"
#include "VulkanContext.h"
#include "Ethane/GfxBackend/GfxObject.h"

namespace Ethane {
namespace ImageUtils {

    inline VkFormat VulkanImageFormat(ImageFormat format)
    {
        switch (format)
        {
        case ImageFormat::RED32F:          return VK_FORMAT_R32_SFLOAT;
        case ImageFormat::RG16F:           return VK_FORMAT_R16G16_SFLOAT;
        case ImageFormat::RG32F:           return VK_FORMAT_R32G32_SFLOAT;
        case ImageFormat::SRGB:            return VK_FORMAT_R8G8B8_SRGB;
        case ImageFormat::RGB:             return VK_FORMAT_R8G8B8_UNORM;
        case ImageFormat::BGR:             return VK_FORMAT_B8G8R8_UNORM;
        case ImageFormat::RGBA:            return VK_FORMAT_R8G8B8A8_UNORM;
        case ImageFormat::RGBA16F:         return VK_FORMAT_R16G16B16A16_SFLOAT;
        case ImageFormat::RGBA32F:         return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ImageFormat::BGRA:            return VK_FORMAT_B8G8R8A8_UNORM;
        case ImageFormat::DEPTH32F:        return VK_FORMAT_D32_SFLOAT;
        case ImageFormat::DEPTH24STENCIL8:  return VK_FORMAT_D24_UNORM_S8_UINT;
        case ImageFormat::DEPTH32FSTENCIL8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case ImageFormat::Depth:
            ETH_CORE_ASSERT(false, "ImageFormat::Depth depends on the device: use the VulkanPhysicalDevice overload");
            return VK_FORMAT_UNDEFINED;
        default: break;
        }
        ETH_CORE_ASSERT(false);
        ETH_CORE_WARN("Image format is {0}", (uint8_t)format);
        return VK_FORMAT_UNDEFINED;
    }

    inline ImageFormat VulkanImageFormatToImageFormat(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R32_SFLOAT:              return ImageFormat::RED32F;
            case VK_FORMAT_R16G16_SFLOAT:           return ImageFormat::RG16F;
            case VK_FORMAT_R32G32_SFLOAT:           return ImageFormat::RG32F;
            case VK_FORMAT_R8G8B8A8_UNORM:          return ImageFormat::RGBA;
            case VK_FORMAT_R16G16B16A16_SFLOAT:     return ImageFormat::RGBA16F;
            case VK_FORMAT_R32G32B32A32_SFLOAT:     return ImageFormat::RGBA32F;
            case VK_FORMAT_R8G8B8_SRGB:             return ImageFormat::SRGB;
            case VK_FORMAT_R8G8B8_UNORM:            return ImageFormat::RGB;
            case VK_FORMAT_B8G8R8_UNORM:            return ImageFormat::BGR;
            case VK_FORMAT_B8G8R8A8_UNORM:          return ImageFormat::BGRA;
            case VK_FORMAT_D32_SFLOAT:              return ImageFormat::DEPTH32F;
            case VK_FORMAT_D24_UNORM_S8_UINT:       return ImageFormat::DEPTH24STENCIL8;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:      return ImageFormat::DEPTH32FSTENCIL8;
            default: break;
        }
        ETH_CORE_ASSERT(false);
        ETH_CORE_WARN("Image format is {0}", format);
        return ImageFormat::None;
    }

    // Same as VulkanImageFormat(), but also resolves the device-dependent defaults (ImageFormat::Depth)
    inline vk::Format VulkanImageFormat(ImageFormat format, const VulkanPhysicalDevice& physicalDevice)
    {
        if (format == ImageFormat::Depth) {
            return physicalDevice.GetDepthFormat();
        }
        return static_cast<vk::Format>(VulkanImageFormat(format));
    }

    inline bool IsDepthFormat(vk::Format format)
    {
        switch (format)
        {
        case vk::Format::eD16Unorm:
        case vk::Format::eX8D24UnormPack32:
        case vk::Format::eD32Sfloat:
        case vk::Format::eD16UnormS8Uint:
        case vk::Format::eD24UnormS8Uint:
        case vk::Format::eD32SfloatS8Uint:
            return true;
        default:
            return false;
        }
    }

    inline bool HasStencilComponent(vk::Format format)
    {
        return format == vk::Format::eS8Uint || format == vk::Format::eD16UnormS8Uint
            || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
    }

    // Aspects of a depth/stencil attachment view: all the format has
    inline vk::ImageAspectFlags DepthAttachmentAspect(vk::Format format)
    {
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eDepth;
        if (HasStencilComponent(format)) {
            aspect |= vk::ImageAspectFlagBits::eStencil;
        }
        return aspect;
    }

    inline VkImageLayout VulkanImageLayout(ImageLayout layout)
    {
        switch (layout)
        {
            case ImageLayout::Undefined:          return VK_IMAGE_LAYOUT_UNDEFINED;
            case ImageLayout::General:            return VK_IMAGE_LAYOUT_GENERAL;
            case ImageLayout::ShaderRead:         return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case ImageLayout::PresentSRC:         return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        ETH_CORE_ASSERT(false);
        ETH_CORE_WARN("Image layout is {0}", (uint8_t)layout);
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    inline VkAccessFlagBits VulkanAccessMask(AccessMask mask)
    {
        switch (mask)
        {
            case AccessMask::None:                          return VK_ACCESS_NONE;
            case AccessMask::ShaderRead:                    return VK_ACCESS_SHADER_READ_BIT;
            case AccessMask::ShaderWrite:                   return VK_ACCESS_SHADER_WRITE_BIT;
            case AccessMask::ColorRead:                     return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            case AccessMask::ColorWrite:                    return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            case AccessMask::TransferRead:                  return VK_ACCESS_TRANSFER_READ_BIT;
            case AccessMask::TransferWrite:                 return VK_ACCESS_TRANSFER_WRITE_BIT;
            case AccessMask::MemoryRead:                    return VK_ACCESS_MEMORY_READ_BIT;
            case AccessMask::MemoryWrite:                   return VK_ACCESS_MEMORY_WRITE_BIT;
        }
        ETH_CORE_ASSERT(false);
        ETH_CORE_WARN("Access mask is {0}", (uint8_t)mask);
        return VK_ACCESS_NONE;
    }

    inline VkPipelineStageFlagBits VulkanPipelineStage(PipelineStage stage)
    {
        switch (stage)
        {
            case PipelineStage::None:                       return VK_PIPELINE_STAGE_NONE;
            case PipelineStage::PipeTop:                    return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            case PipelineStage::VertexShader:               return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            case PipelineStage::FragmentShader:             return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            case PipelineStage::ComputeShader:              return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            case PipelineStage::Transfer:                   return VK_PIPELINE_STAGE_TRANSFER_BIT;
            case PipelineStage::PipeBottom:                 return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        ETH_CORE_ASSERT(false);
        ETH_CORE_WARN("Pipeline stage is {0}", (uint8_t)stage);
        return VK_PIPELINE_STAGE_NONE;
    }
}}