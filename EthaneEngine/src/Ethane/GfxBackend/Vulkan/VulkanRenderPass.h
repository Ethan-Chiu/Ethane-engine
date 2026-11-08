#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"

namespace Ethane {

	class VulkanRenderPass : public IRenderPass
	{
	public:
        VulkanRenderPass() = delete;
		VulkanRenderPass(const VulkanDevice* device, const std::vector<ImageFormat>& colorAttachmentFormats, ImageFormat depthAttachmentFormat, bool clearOnLoad, bool finalPresent = false);
		virtual ~VulkanRenderPass() = default;

		void Begin(const vk::raii::CommandBuffer& cmdBuffer, uint32_t width, uint32_t height, vk::Framebuffer frameBuffer) const;
		void End(const vk::raii::CommandBuffer& cmdBuffer) const;

		vk::RenderPass GetHandle() const { return *m_RenderPass; }
		uint8_t GetAttachmentCount() const { return m_AttachmentCount; }

	private:
        const VulkanDevice* m_Device;
        VulkanLiveResource m_LiveResource;
		vk::raii::RenderPass m_RenderPass = nullptr;

		uint8_t m_AttachmentCount = 0;
	};

}
