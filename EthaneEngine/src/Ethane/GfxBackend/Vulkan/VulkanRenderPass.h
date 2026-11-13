#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"

namespace Ethane {

	class VulkanRenderPass : public IRenderPass
	{
	public:
        VulkanRenderPass() = delete;
		VulkanRenderPass(const VulkanDevice* device, const std::vector<ImageFormat>& colorAttachmentFormats, ImageFormat depthAttachmentFormat, bool clearOnLoad, bool finalPresent = false);
		// Frames in flight may still use it: destroyed once they've finished
		virtual ~VulkanRenderPass() { m_Device->Retire(std::move(m_RenderPass)); }

		// Clears the color attachments to `clearColor` and the depth attachment to 1.0
		void Begin(const vk::raii::CommandBuffer& cmdBuffer, uint32_t width, uint32_t height, vk::Framebuffer frameBuffer,
		           const std::array<float, 4>& clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }) const;
		void End(const vk::raii::CommandBuffer& cmdBuffer) const;

		vk::RenderPass GetHandle() const { return *m_RenderPass; }
		// Color attachments only (the depth attachment, if any, comes after them)
		uint8_t GetAttachmentCount() const { return m_AttachmentCount; }
		bool HasDepth() const { return m_HasDepth; }

	private:
        const VulkanDevice* m_Device;
        VulkanLiveResource m_LiveResource;
		vk::raii::RenderPass m_RenderPass = nullptr;

		uint8_t m_AttachmentCount = 0;
		bool m_HasDepth = false;
	};

}
