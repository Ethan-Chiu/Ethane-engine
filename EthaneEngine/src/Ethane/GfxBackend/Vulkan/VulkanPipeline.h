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

		void Bind(const vk::raii::CommandBuffer& cmdBuffer, vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) const;

		// Getter
		vk::Pipeline GetVulkanPipeline() const { return *m_GraphicsPipeline; }
		vk::PipelineLayout GetPipelineLayout() const { return *m_PipelineLayout; }
		const std::vector<vk::PushConstantRange>& GetPushConstantRanges() const { return m_PushConstantRanges; }
		virtual PipelineSpecification& GetSpecification() override { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const override { return m_Specification; }
    private:
        void Create();
        void Destroy();
		static vk::CullModeFlags VulkanCullMode(CullMode cullMode);

	private:
		// Holds references to the shader and render pass, so they outlive this pipeline
		PipelineSpecification m_Specification;

        const VulkanDevice* m_Device = nullptr;
        VulkanLiveResource m_LiveResource;
		std::vector<vk::PushConstantRange> m_PushConstantRanges; // from the shader's reflection, as in the layout

        // NOTE: declaration order matters, members are destroyed in reverse: pipeline -> layout
		vk::raii::PipelineLayout m_PipelineLayout = nullptr;
		vk::raii::Pipeline m_GraphicsPipeline = nullptr;
	};

}
