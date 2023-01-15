#pragma once

#include "Ethane/Renderer/Texture.h"

#include "VulkanImage.h"

namespace Ethane {

	// virtual uint32_t GetRendererID() const = 0;

	// virtual bool operator==(const Texture & other) const = 0;

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const std::string& path); // TextureProperties properties
		VulkanTexture2D(uint32_t width, uint32_t height); // ImageFormat format, TextureProperties properties, const void* data
		~VulkanTexture2D() override = default;

		void Cleanup();
		// virtual void Resize(uint32_t width, uint32_t height) override;

		void Invalidate() {};

		virtual void SetData(void* data, uint32_t size) override;

		// Getter
		virtual uint32_t GetWidth() const override { return m_Width; } 
		virtual uint32_t GetHeight() const override { return m_Height; } 
		// virtual ImageFormat GetFormat() const override { return m_Format; }
		const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_Image->GetDescriptor(); }
		VkImageView GetImageView() { return m_Image->GetImageInfo().ImageView; }
		VkSampler GetImageSampler() { return m_Image->GetImageInfo().Sampler; }

		virtual void Bind(uint32_t slot = 0) const override {}
		virtual uint32_t GetRendererID() const override { return 0; };
		virtual bool operator==(const Texture& other) const { return this->m_Path == ((VulkanTexture2D&)other).m_Path; };
		virtual Ref<Image2D> GetImage() const override { return std::dynamic_pointer_cast<Image2D>(m_Image); } // TODO: 
		// const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_Image.As<VulkanImage2D>()->GetDescriptor(); }

		// void Lock() override;
		// void Unlock() override;

		// Buffer GetWriteableBuffer() override;
		// bool Loaded() const override { return m_ImageData; }
		// const std::string& GetPath() const override;
		// uint32_t GetMipLevelCount() const override;
		// virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		// void GenerateMips();

		// virtual uint64_t GetHash() const { return (uint64_t)m_Image.As<VulkanImage2D>()->GetDescriptor().imageView; }
	private:
		void CreateTextureSampler();
	private:
		std::string m_Path;
		uint32_t m_Width, m_Height;
		uint32_t m_ChannelCount;

		Ref<VulkanImage2D> m_Image = nullptr;
		VkSampler m_TextureSampler;

		// Buffer m_ImageData

		// TextureProperties m_Properties;

		// ImageFormat m_Format = ImageFormat::None;

		VkDescriptorImageInfo m_DescriptorInfo{};
	};
}