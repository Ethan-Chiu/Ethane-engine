#pragma once

#include "Vulkan.h"
#include "Ethane/Renderer/Pipeline.h"

namespace Ethane {

	class VulkanPipeline : public Pipeline
	{

	public:
		VulkanPipeline() = default;
		VulkanPipeline(VkRenderPass renderPass, VertexBufferLayout layout);
		VulkanPipeline(PipelineSpecification spec) {};
		~VulkanPipeline();
		void Cleanup();

		void Invalidate() override;

		// remove after
		virtual void Bind() override {}

		// Getter
		VkPipeline GetVulkanPipeline() { return m_GraphicsPipeline; }
		virtual PipelineSpecification& GetSpecification() { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const { return m_Specification; }
	private:
		PipelineSpecification m_Specification;

		VkPipeline m_GraphicsPipeline;
		VkPipelineLayout m_PipelineLayout; // test 

		VkRenderPass m_RenderPass; // TODO: remove this temp
		VertexBufferLayout m_Layout; // TODO: remove this temp

	};

}