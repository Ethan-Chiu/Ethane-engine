#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"

namespace Ethane {

	class VulkanRenderPass : public IRenderPass
	{
	public:
		VulkanRenderPass(const VulkanDevice* device, const std::vector<ImageFormat>& colorAttachmentFormats, ImageFormat depthAttachmentFormat, bool clearOnLoad, bool finalPresent = false);
		virtual ~VulkanRenderPass();
        
		void Begin(VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, VkFramebuffer frameBuffer);
		void End(VkCommandBuffer cmdBuffer);

		VkRenderPass GetHandle() const { return m_RenderPass; }
		uint8_t GetAttachmentCount() const { return m_AttachmentCount; }
	
	private:
        const VulkanDevice* m_Device;
		VkRenderPass m_RenderPass;

		uint8_t m_AttachmentCount = 0;
	};

}