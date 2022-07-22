#include "ethpch.h"
#include "VulkanTexture.h"

#include "VulkanContext.h";

#include "stb_image.h"

namespace Ethane{

	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		// create command buffer //TODO: change command buffer to be a param
		VkCommandBuffer commandBuffer = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };

		region.imageExtent.width = width;
		region.imageExtent.height = height;
		region.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		// submit command buffer
		VulkanContext::GetDevice()->SubmitCommandBuffer(commandBuffer);
	}

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		// create command buffer //TODO: change command buffer to be a param
		VkCommandBuffer commandBuffer = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);

		VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		// submit command buffer
		VulkanContext::GetDevice()->SubmitCommandBuffer(commandBuffer);
	}

	VulkanTexture2D::VulkanTexture2D(const std::string& path)
		:m_Path(path)
	{
		ETH_PROFILE_FUNCTION();

		int width, height, channels;
		stbi_uc* data = nullptr;
		{
			ETH_PROFILE_SCOPE("stbi_load _ OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
			data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		}
		ETH_CORE_ASSERT(data, "Failed to load image!");
		m_Width = width;
		m_Height = height;
		VkDeviceSize imageSize = m_Width * m_Height * 4;
		ETH_CORE_TRACE("w: {0}, h: {1}, ch: {2}", m_Width, m_Height, channels);

		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		// create staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Utils::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		// copy data to staging buffer
		void* dstData;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &dstData);
		memcpy(dstData, data, imageSize);
		vkUnmapMemory(device, stagingBufferMemory);

		stbi_image_free(data);

		// Create Imgae
		ImageSpecification imageSpec;
		imageSpec.Width = m_Width;
		imageSpec.Height = m_Height;
		m_Image = CreateRef<VulkanImage2D>(imageSpec);

		
		VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
		// CreateImage(m_Width, m_Height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		// 	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);
		VkImage textureImage = m_Image->GetImageInfo().Image;

		TransitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		CopyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));

		TransitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Create ImageView
		// m_TextureImageView = CreateImageView(m_TextureImage, format);

		// Create Sampler
		// CreateTextureSampler();

		// m_DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// m_DescriptorInfo.imageView = m_TextureImageView;
		// m_DescriptorInfo.sampler = m_TextureSampler;

		// cleanup staging buffer
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height)
	{
		ETH_PROFILE_FUNCTION();

		m_Width = width;
		m_Height = height;
		m_ChannelCount = 4;
		VkDeviceSize imageSize = m_Width * m_Height * m_ChannelCount;
		ETH_CORE_TRACE("w: {0}, h: {1}, ch: {2}", m_Width, m_Height, m_ChannelCount);

		// Create Imgae
		ImageSpecification imageSpec;
		imageSpec.Width = m_Width;
		imageSpec.Height = m_Height;
		m_Image = CreateRef<VulkanImage2D>(imageSpec);
	}

	void VulkanTexture2D::SetData(void* data, uint32_t imageSize)
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		// create staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Utils::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		// copy data to staging buffer
		void* dstData;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &dstData);
		memcpy(dstData, data, imageSize);
		vkUnmapMemory(device, stagingBufferMemory);

		// transition
		VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
		VkImage textureImage = m_Image->GetImageInfo().Image;
		TransitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));
		TransitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// cleanup staging buffer
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void VulkanTexture2D::Cleanup()
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		m_Image->Destroy();
		// vkDestroySampler(device, m_TextureSampler, nullptr);
		// 
		// vkDestroyImageView(device, m_TextureImageView, nullptr);
		// vkDestroyImage(device, m_TextureImage, nullptr);
		// 
		// vkFreeMemory(device, m_TextureImageMemory, nullptr);
	}

	void VulkanTexture2D::CreateTextureSampler() {
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		
		VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
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
		
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &m_TextureSampler));
	}
}