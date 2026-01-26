#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"

namespace Ethane {

	class VulkanPipeline : public IPipeline
	{

	public:
		VulkanPipeline() = delete;
		VulkanPipeline(const VulkanDevice* device, const PipelineSpecification& spec);
		virtual ~VulkanPipeline() override;

		void Bind(VkCommandBuffer cmdBuffer, VkPipelineBindPoint bindPoint);

		// Getter
		VkPipeline GetVulkanPipeline() { return m_GraphicsPipeline; }
		VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
		virtual PipelineSpecification& GetSpecification() override { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const override { return m_Specification; }
    private:
        void Create();
        void Destroy();
		VkCullModeFlags VulkanCullMode(CullMode cullMode);

	private:
		PipelineSpecification m_Specification;

        const VulkanDevice* m_Device = nullptr;
		VkPipeline m_GraphicsPipeline;
		VkPipelineLayout m_PipelineLayout; 
	};

}