#pragma once

#include "Vulkan.h"

#include "Ethane/Renderer/RenderPass.h"

namespace Ethane {

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass() = default;
		VulkanRenderPass(const RenderPassSpecification& spec, VkRenderPass renderPass)
			:m_Specification(spec), m_RenderPass(renderPass)
		{
		} // TODO: temp

		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass() {};

		void Create(bool hasDepth);
		void Destroy();
		void Begin(VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, VkFramebuffer frameBuffer);
		void End(VkCommandBuffer cmdBuffer);

		// Getter
		virtual RenderPassSpecification& GetSpecification() override { return m_Specification; }
		virtual const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

		VkRenderPass GetHandle() { return m_RenderPass; }
	private:
		RenderPassSpecification m_Specification;

		VkRenderPass m_RenderPass;
	};

}
