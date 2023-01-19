#pragma once

#include "Ethane/Renderer/Texture.h"

#include "VulkanImage.h"

namespace Ethane {

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const std::string& path); // TextureProperties properties
		VulkanTexture2D(uint32_t width, uint32_t height); // ImageFormat format, TextureProperties properties, const void* data
		~VulkanTexture2D() override = default;

		void Cleanup();

		void Invalidate() {};

		virtual void SetData(void* data, uint32_t size) override;

		// Getter
		virtual uint32_t GetWidth() const override { return m_Width; } 
		virtual uint32_t GetHeight() const override { return m_Height; } 
		const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_Image->GetDescriptor(); }
		VkImageView GetImageView() { return m_Image->GetImageInfo().ImageView; }
		VkSampler GetImageSampler() { return m_Image->GetImageInfo().Sampler; }

		virtual void Bind(uint32_t slot = 0) const override {}
		virtual bool operator==(const Texture& other) const { return this->m_Path == ((VulkanTexture2D&)other).m_Path; };
		virtual Ref<Image2D> GetImage() const override { return std::dynamic_pointer_cast<Image2D>(m_Image); } // TODO: 
		
	private:
		void CreateTextureSampler();
	private:
		std::string m_Path;
		uint32_t m_Width, m_Height;
		uint32_t m_ChannelCount;

		Ref<VulkanImage2D> m_Image = nullptr;
		VkSampler m_TextureSampler;

		VkDescriptorImageInfo m_DescriptorInfo{};
	};
}