#pragma once

#include "Vulkan.h"
#include "Ethane/Renderer/Pipeline.h"

namespace Ethane {

	class VulkanPipeline : public Pipeline
	{

	public:
		VulkanPipeline() = default;
		VulkanPipeline(const PipelineSpecification& spec);
		// VulkanPipeline(Ref<VulkanShader> vulkanShader, VkRenderPass renderPass, VertexBufferLayout layout);
		virtual ~VulkanPipeline() override;
		void Cleanup();

		void Invalidate() override;

		// remove after
		virtual void Bind() override {}

		// Getter
		VkPipeline GetVulkanPipeline() { return m_GraphicsPipeline; }
		VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
		virtual PipelineSpecification& GetSpecification() override { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const override { return m_Specification; }
	private:
		PipelineSpecification m_Specification;

		VkPipeline m_GraphicsPipeline;
		VkPipelineLayout m_PipelineLayout; // test 



		// Ref<VulkanShader> m_VulkanShader; // TODO: remove test
		// VkRenderPass m_RenderPass; // TODO: remove this temp
		// VertexBufferLayout m_Layout; // TODO: remove this temp

	};

}