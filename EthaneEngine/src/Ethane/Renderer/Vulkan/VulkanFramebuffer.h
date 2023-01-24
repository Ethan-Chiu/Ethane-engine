#pragma once

#include "Ethane/Renderer/Framebuffer.h"

#include "Vulkan.h"

#include "VulkanImage.h"

#include "VulkanTexture.h"

namespace Ethane {

	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer(const FramebufferSpecification& spec);

		virtual void Resize(uint32_t width, uint32_t height) override;
		// virtual void AddResizeCallback(const std::function<void(Ref<Framebuffer>)>& func) override;

		virtual void Bind() override {}
		virtual void Unbind() override {}

		void Invalidate();

		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override { return 0; };

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override {};

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { return 0; };

		// Getter
		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
		virtual uint32_t GetWidth()  const { return m_Width; }
		virtual uint32_t GetHeight() const { return m_Height; }
		virtual uint32_t GetRendererID() const { return m_RendererID; }
		virtual uint32_t GetColorAttachmentRendererID() const { return 0; }
		virtual uint32_t GetDepthAttachmentRendererID() const { return 0; }

		virtual Ref<Image2D> GetImage(uint32_t attachmentIndex = 0) const { ETH_CORE_ASSERT(attachmentIndex < m_AttachmentImages.size()); return m_AttachmentImages[attachmentIndex]; }
		virtual Ref<VulkanTexture2D> GetTexture(uint32_t attachmentIndex = 0) const { ETH_CORE_ASSERT(attachmentIndex < m_AttachmentTextures.size()); return m_AttachmentTextures[attachmentIndex]; }
		virtual Ref<Image2D> GetDepthImage() const { return m_DepthAttachmentImage; }
		size_t GetColorAttachmentCount() const { return m_Specification.SwapChainTarget ? 1 : m_AttachmentImages.size(); }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkFramebuffer GetVulkanFramebuffer() const { return m_Framebuffer; }
		const std::vector<VkClearValue>& GetVulkanClearValues() const { return m_ClearValues; }

		bool HasDepthAttachment() const { return (bool)m_DepthAttachmentImage; }

	private:
		FramebufferSpecification m_Specification;
		uint32_t m_RendererID = 0;
		uint32_t m_Width = 0, m_Height = 0;

		std::vector<Ref<Image2D>> m_AttachmentImages;
		std::vector<Ref<VulkanTexture2D>> m_AttachmentTextures;
		Ref<Image2D> m_DepthAttachmentImage;
		Ref<VulkanTexture2D> m_DepthAttachmentTexture;

		std::vector<VkClearValue> m_ClearValues;

		VkRenderPass m_RenderPass = nullptr;
		VkFramebuffer m_Framebuffer = nullptr;

		std::vector<std::function<void(Ref<Framebuffer>)>> m_ResizeCallbacks;
	};

}