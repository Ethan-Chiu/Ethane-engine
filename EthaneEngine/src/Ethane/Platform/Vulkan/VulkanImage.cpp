#include "ethpch.h"
#include "VulkanContext.h"

#include "VulkanImage.h"

// #include "VulkanRenderer.h

namespace Ethane {

	// static std::map<VkImage, WeakRef<VulkanImage2D>> s_ImageReferences;

	VulkanImage2D::VulkanImage2D(ImageSpecification specification, void* buffer)
		: m_Specification(specification)
	{
		ETH_CORE_ASSERT(m_Specification.Width > 0 && m_Specification.Height > 0);
		ETH_CORE_TRACE("VulkanImage2D::Invalidate ({0})", m_Specification.DebugName);

		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		if (m_Specification.Usage == ImageUsage::Attachment)
		{
			if (Utils::IsDepthFormat(m_Specification.Format))
				usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			else
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		else if (m_Specification.Usage == ImageUsage::Texture)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		else if (m_Specification.Usage == ImageUsage::Storage)
		{
			usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

		// Create Image
		CreateVulkanImage(device,
			m_Specification.Width,
			m_Specification.Height,
			m_Specification.Mips,
			m_Specification.Layers,
			vulkanFormat,
			VK_IMAGE_TILING_OPTIMAL,
			usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// s_ImageReferences[m_Info.Image] = this;

		VkImageAspectFlags aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (m_Specification.Format == ImageFormat::DEPTH24STENCIL8)
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		// Create Image view
		CreateImageView(device, vulkanFormat, aspectMask);
		
		// TODO: move sampler away
		// TODO: Renderer should contain some kind of sampler cache
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &m_Info.Sampler));

		// if (m_Specification.Usage == ImageUsage::Storage)
		// {
		// 	// Transition image to GENERAL layout
		// 	VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);
		// 
		// 	VkImageSubresourceRange subresourceRange = {};
		// 	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// 	subresourceRange.baseMipLevel = 0;
		// 	subresourceRange.levelCount = m_Specification.Mips;
		// 	subresourceRange.layerCount = m_Specification.Layers;
		// 
		// 	Utils::InsertImageMemoryBarrier(commandBuffer, m_Info.Image,
		// 		0, 0,
		// 		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		// 		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		// 		subresourceRange);
		// 
		// 	VulkanContext::GetCurrentDevice()->FlushCommandBuffer(commandBuffer);
		// }

		UpdateDescriptorImageInfo();
	}

	VulkanImage2D::VulkanImage2D(uint32_t width, uint32_t height, uint32_t mip, uint32_t layers, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlag, VkImageAspectFlags aspectFlag, bool createView)
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		CreateVulkanImage(device, width, height, mip, layers, format, tiling, usage, memoryFlag);
		if (createView) {
			CreateImageView(device, format, aspectFlag);
		}
	}

	Ref<VulkanImage2D> VulkanImage2D::Create(uint32_t width, uint32_t height, uint32_t mip, uint32_t layers, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlag, VkImageAspectFlags aspectFlag, bool createView)
	{
		return CreateRef<VulkanImage2D>(width, height, mip, layers, format, tiling, usage, memoryFlag, aspectFlag, createView);
	}

	VulkanImage2D::~VulkanImage2D()
	{
		if (m_Info.Image)
		{
			Destroy();
			// m_PerLayerImageViews.clear();
		}
	}

	void VulkanImage2D::CreateVulkanImage(VkDevice device, uint32_t width, uint32_t height, uint32_t mip, uint32_t layers, VkFormat format, VkImageTiling tiling, 
		VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlag)
	{
		// Create Image
		VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mip;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = 0; // Optional
		VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &m_Info.Image));

		//Allocate image memory
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, m_Info.Image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = Utils::FindMemoryType(memRequirements.memoryTypeBits, memoryFlag);
		VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &m_ImageMemory));

		// Bind the memory
		VK_CHECK_RESULT(vkBindImageMemory(device, m_Info.Image, m_ImageMemory, 0));
	}

	void VulkanImage2D::CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectMask)
	{
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.image = m_Info.Image;
		viewInfo.viewType = m_Specification.Layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectMask;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = m_Specification.Mips;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = m_Specification.Layers;
		viewInfo.flags = 0;
		VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &m_Info.ImageView));
	}


	void VulkanImage2D::Destroy()
	{
		if (m_Info.Image == nullptr)
			return;

		auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		vkDestroySampler(device, m_Info.Sampler, nullptr);

		if (m_Info.ImageView) {
			vkDestroyImageView(device, m_Info.ImageView, nullptr);
			ETH_CORE_WARN("VulkanImage2D::Release ImageView = {0}", (const void*)m_Info.ImageView);
		}

		if (m_ImageMemory) {
			vkFreeMemory(device, m_ImageMemory, nullptr);
			m_ImageMemory = nullptr;
		}

		// for (auto& view : layerViews)
		// {
		// 	if (view)
		// 		vkDestroyImageView(vulkanDevice, view, nullptr);
		// }

		if (m_Info.Image)
			vkDestroyImage(device, m_Info.Image, nullptr);
		

		m_Info.Image = nullptr;
		m_Info.ImageView = nullptr;
		m_Info.Sampler = nullptr;
		
		// m_PerLayerImageViews.clear();

		// m_MipImageViews.clear();
	}

	void VulkanImage2D::UpdateDescriptorImageInfo()
	{
		if (m_Specification.Format == ImageFormat::DEPTH24STENCIL8 || m_Specification.Format == ImageFormat::DEPTH32F)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		else if (m_Specification.Usage == ImageUsage::Storage)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		else
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (m_Specification.Usage == ImageUsage::Storage)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;

		ETH_CORE_TRACE("VulkanImage2D::UpdateDescriptorImageInfo to ImageView = {0}", (const void*)m_Info.ImageView);
	}

	// const std::map<VkImage, WeakRef<VulkanImage2D>>& VulkanImage2D::GetImageRefs()
	// {
	// 	return s_ImageReferences;
	// }


	// void VulkanImage2D::CreatePerLayerImageViews()
	// {
	// 	HZ_CORE_ASSERT(m_Specification.Layers > 1);
	// 
	// 	VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
	// 
	// 	VkImageAspectFlags aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	// 	if (m_Specification.Format == ImageFormat::DEPTH24STENCIL8)
	// 		aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	// 
	// 	const VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);
	// 
	// 	m_PerLayerImageViews.resize(m_Specification.Layers);
	// 	for (uint32_t layer = 0; layer < m_Specification.Layers; layer++)
	// 	{
	// 		VkImageViewCreateInfo imageViewCreateInfo = {};
	// 		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	// 		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	// 		imageViewCreateInfo.format = vulkanFormat;
	// 		imageViewCreateInfo.flags = 0;
	// 		imageViewCreateInfo.subresourceRange = {};
	// 		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	// 		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	// 		imageViewCreateInfo.subresourceRange.levelCount = m_Specification.Mips;
	// 		imageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
	// 		imageViewCreateInfo.subresourceRange.layerCount = 1;
	// 		imageViewCreateInfo.image = m_Info.Image;
	// 		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_PerLayerImageViews[layer]));
	// 	}
	// }

	// VkImageView VulkanImage2D::GetMipImageView(uint32_t mip)
	// {
	// 	if (m_MipImageViews.find(mip) == m_MipImageViews.end())
	// 	{
	// 		Ref<VulkanImage2D> instance = this;
	// 		Renderer::Submit([instance, mip]() mutable
	// 			{
	// 				if (m_MipImageViews.find(mip) == m_MipImageViews.end())
	// 				{
	// 					VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
	// 
	// 					VkImageAspectFlags aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	// 					if (m_Specification.Format == ImageFormat::DEPTH24STENCIL8)
	// 						aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	// 
	// 					VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);
	// 
	// 					m_PerLayerImageViews.resize(m_Specification.Layers);
	// 					VkImageViewCreateInfo imageViewCreateInfo = {};
	// 					imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	// 					imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	// 					imageViewCreateInfo.format = vulkanFormat;
	// 					imageViewCreateInfo.flags = 0;
	// 					imageViewCreateInfo.subresourceRange = {};
	// 					imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	// 					imageViewCreateInfo.subresourceRange.baseMipLevel = mip;
	// 					imageViewCreateInfo.subresourceRange.levelCount = 1;
	// 					imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	// 					imageViewCreateInfo.subresourceRange.layerCount = 1;
	// 					imageViewCreateInfo.image = m_Info.Image;
	// 
	// 					VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_MipImageViews[mip]));
	// 				}
	// 				return m_MipImageViews.at(mip);
	// 			});
	// 		return nullptr;
	// 	}
	// 
	// 	return m_MipImageViews.at(mip);
	// }

	// void VulkanImage2D::RT_CreatePerSpecificLayerImageViews(const std::vector<uint32_t>& layerIndices)
	// {
	// 	HZ_CORE_ASSERT(m_Specification.Layers > 1);
	// 
	// 	VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
	// 
	// 	VkImageAspectFlags aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	// 	if (m_Specification.Format == ImageFormat::DEPTH24STENCIL8)
	// 		aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	// 
	// 	const VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);
	// 
	// 	//HZ_CORE_ASSERT(m_PerLayerImageViews.size() == m_Specification.Layers);
	// 	if (m_PerLayerImageViews.empty())
	// 		m_PerLayerImageViews.resize(m_Specification.Layers);
	// 
	// 	for (uint32_t layer : layerIndices)
	// 	{
	// 		VkImageViewCreateInfo imageViewCreateInfo = {};
	// 		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	// 		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	// 		imageViewCreateInfo.format = vulkanFormat;
	// 		imageViewCreateInfo.flags = 0;
	// 		imageViewCreateInfo.subresourceRange = {};
	// 		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	// 		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	// 		imageViewCreateInfo.subresourceRange.levelCount = m_Specification.Mips;
	// 		imageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
	// 		imageViewCreateInfo.subresourceRange.layerCount = 1;
	// 		imageViewCreateInfo.image = m_Info.Image;
	// 		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_PerLayerImageViews[layer]));
	// 	}
	// 
	// }
}
