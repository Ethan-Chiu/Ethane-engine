#pragma once

#include "Vulkan.h"
#include "Ethane/Renderer/Pipeline.h"

#include "VulkanCommandBuffer.h"

namespace Ethane {

	class VulkanPipeline : public Pipeline
	{

	public:
		VulkanPipeline() = default;
		VulkanPipeline(const PipelineSpecification& spec);
		virtual ~VulkanPipeline() override;
		
		void Destroy();

		void Create() override;

		// remove after
		virtual void Bind() override {}

		void Bind(Ref<VulkanCommandBuffer> cmdBuffer, VkPipelineBindPoint bindPoint);

		// Getter
		VkPipeline GetVulkanPipeline() { return m_GraphicsPipeline; }
		VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
		virtual PipelineSpecification& GetSpecification() override { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const override { return m_Specification; }
	private:
		PipelineSpecification m_Specification;

		VkPipeline m_GraphicsPipeline;
		VkPipelineLayout m_PipelineLayout; // test 
	};

}