#pragma once

#include "Ethane/Renderer/Image.h"

#include <vulkan/vulkan.h>

namespace Ethane {

	struct VulkanImageInfo
	{
		VkImage Image = nullptr;
		VkImageView ImageView = nullptr;
		VkSampler Sampler = nullptr;
	};

	class VulkanImage2D  : public Image2D
	{
	public:
		VulkanImage2D(ImageSpecification specification, void* buffer = nullptr);
		virtual ~VulkanImage2D() override;

		virtual void Invalidate() override;
		virtual void Cleanup() override;

		void UpdateDescriptorImageInfo();

		// Getter
		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual float GetAspectRatio() const override { return (float)m_Specification.Width / (float)m_Specification.Height; }
		virtual ImageSpecification& GetSpecification() override { return m_Specification; }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }
		
		VulkanImageInfo& GetImageInfo() { return m_Info; }
		const VulkanImageInfo& GetImageInfo() const { return m_Info; }
		const VkDescriptorImageInfo& GetDescriptor() { return m_DescriptorImageInfo; }

		// virtual void CreatePerLayerImageViews() override;
		// void RT_CreatePerLayerImageViews();
		// void RT_CreatePerSpecificLayerImageViews(const std::vector<uint32_t>& layerIndices);

		// virtual VkImageView GetLayerImageView(uint32_t layer)
		// {
		// 	ETH_CORE_ASSERT(layer < m_PerLayerImageViews.size());
		// 	return m_PerLayerImageViews[layer];
		// }

		// VkImageView GetMipImageView(uint32_t mip);
		// VkImageView RT_GetMipImageView(uint32_t mip);


		// virtual uint64_t GetHash() const override { return (uint64_t)m_Info.Image; }


	private:
		ImageSpecification m_Specification;

		VulkanImageInfo m_Info;
		VkDeviceMemory m_ImageMemory;

		VkDescriptorImageInfo m_DescriptorImageInfo = {};
		// std::vector<VkImageView> m_PerLayerImageViews;
		// std::map<uint32_t, VkImageView> m_MipImageViews;
	};

	
	namespace Utils {

		inline VkFormat VulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RED32F:          return VK_FORMAT_R32_SFLOAT;
			case ImageFormat::RG16F:		   return VK_FORMAT_R16G16_SFLOAT;
			case ImageFormat::RG32F:		   return VK_FORMAT_R32G32_SFLOAT;
			case ImageFormat::RGBA:            return VK_FORMAT_R8G8B8A8_UNORM;
			case ImageFormat::RGBA16F:         return VK_FORMAT_R16G16B16A16_SFLOAT;
			case ImageFormat::RGBA32F:         return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ImageFormat::DEPTH32F:        return VK_FORMAT_D32_SFLOAT;
			case ImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;// VulkanContext::GetDevice()->GetPhysicalDevice()->GetDepthFormat();
			}
			ETH_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}

	}

}
