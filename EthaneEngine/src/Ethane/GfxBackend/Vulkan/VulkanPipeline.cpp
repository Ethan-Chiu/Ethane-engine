#include "ethpch.h"
#include "Ethane/GfxBackend/Vulkan/VulkanUtils.h"
#include "Ethane/GfxBackend/Resource.h"
#include "VulkanPipeline.h"
#include "VulkanContext.h"
#include "VulkanShader.h"
#include "VulkanRenderPass.h"

namespace Ethane {

	namespace Utils {
		static VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
		{
			switch (type)
			{
			case ShaderDataType::Float:     return VK_FORMAT_R32_SFLOAT;
			case ShaderDataType::Float2:    return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataType::Float3:    return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataType::Float4:    return VK_FORMAT_R32G32B32A32_SFLOAT;

			case ShaderDataType::Int:		return VK_FORMAT_R32_SINT;
			case ShaderDataType::Int2:		return VK_FORMAT_R32G32_SINT;
			case ShaderDataType::Int3:		return VK_FORMAT_R32G32B32_SINT;
			case ShaderDataType::Int4:		return VK_FORMAT_R32G32B32A32_SINT;

            case ShaderDataType::Bool:      return VK_FORMAT_R32_SINT;

            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
            case ShaderDataType::None:
                break;
			}
			ETH_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}
	}

	VulkanPipeline::VulkanPipeline(const VulkanDevice* device, const PipelineSpecification& spec)
		:m_Specification(spec), m_Device(device)
	{
		Create();
	}

	VulkanPipeline::~VulkanPipeline()
	{
        Destroy();
    }

	void VulkanPipeline::Destroy()
	{
		if (!*m_GraphicsPipeline && !*m_PipelineLayout) {
			return;
		}
        m_Device->Get().waitIdle();
		m_GraphicsPipeline = nullptr;
		m_PipelineLayout = nullptr;
	}

	void VulkanPipeline::Create()
	{
		// Shader Stage
		RefCountPtr<VulkanShader> vulkanShader = dynamic_pointer_cast<VulkanShader>(m_Specification.Shader);
		RefCountPtr<VulkanRenderPass> renderPass = dynamic_pointer_cast<VulkanRenderPass>(m_Specification.RenderPass);

		const auto& shaderStages = vulkanShader->GetPipelineShaderStageCreateInfos();

		// Fixed Function 
		// vertex input
		const VertexBufferLayout& layout = m_Specification.Layout;
		const vk::VertexInputBindingDescription vertexInputBinding{
			.binding   = 0,
			.stride    = layout.GetStride(),
			.inputRate = vk::VertexInputRate::eVertex};

		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
		attributeDescriptions.reserve(layout.GetElementCount());
		uint32_t location = 0;
		for (const auto& element : layout)
		{
			attributeDescriptions.push_back(vk::VertexInputAttributeDescription{
				.location = location,
				.binding  = 0,
				.format   = static_cast<vk::Format>(Utils::ShaderDataTypeToVulkanFormat(element.Type)),
				.offset   = element.Offset});
			location++;
		}
		const vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			.vertexBindingDescriptionCount   = 1,
			.pVertexBindingDescriptions      = &vertexInputBinding,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions    = attributeDescriptions.data()};

		// input assembly
		const vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
			.topology               = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = vk::False};

		// viewport: dynamic state, only the counts are fixed here
		const vk::PipelineViewportStateCreateInfo viewportState{
			.viewportCount = 1,
			.scissorCount  = 1};

		// rasterizer
		const vk::PipelineRasterizationStateCreateInfo rasterizer{
			.depthClampEnable        = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode             = vk::PolygonMode::eFill,
			.cullMode                = VulkanCullMode(m_Specification.CullMode),
			.frontFace               = vk::FrontFace::eCounterClockwise,
			.depthBiasEnable         = vk::False,
			.depthBiasConstantFactor = 0.0f, // Optional
			.depthBiasClamp          = 0.0f, // Optional
			.depthBiasSlopeFactor    = 0.0f, // Optional
			.lineWidth               = 1.0f};

		// multisampling
		const vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples  = vk::SampleCountFlagBits::e1,
			.sampleShadingEnable   = vk::False,
			.minSampleShading      = 1.0f,     // Optional
			.pSampleMask           = nullptr,  // Optional
			.alphaToCoverageEnable = vk::False, // Optional
			.alphaToOneEnable      = vk::False}; // Optional

		// depth and stencil testing
		const vk::PipelineDepthStencilStateCreateInfo depthStencil{
			.depthTestEnable       = vk::True,
			.depthWriteEnable      = vk::True,
			.depthCompareOp        = vk::CompareOp::eLess,
			.depthBoundsTestEnable = vk::False,
			.stencilTestEnable     = vk::False,
			.front                 = {}, // Optional
			.back                  = {}, // Optional
			.minDepthBounds        = 0.0f, // Optional
			.maxDepthBounds        = 1.0f}; // Optional

		// color blending: one (opaque) state per color attachment of the render pass
		using Color = vk::ColorComponentFlagBits;
		const std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(renderPass->GetAttachmentCount(), vk::PipelineColorBlendAttachmentState{
			.blendEnable         = vk::False,
			.srcColorBlendFactor = vk::BlendFactor::eOne,  // Optional
			.dstColorBlendFactor = vk::BlendFactor::eZero, // Optional
			.colorBlendOp        = vk::BlendOp::eAdd,      // Optional
			.srcAlphaBlendFactor = vk::BlendFactor::eOne,  // Optional
			.dstAlphaBlendFactor = vk::BlendFactor::eZero, // Optional
			.alphaBlendOp        = vk::BlendOp::eAdd,      // Optional
			.colorWriteMask      = Color::eR | Color::eG | Color::eB | Color::eA});
		const vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable   = vk::False,
			.logicOp         = vk::LogicOp::eCopy, // Optional
			.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size()),
			.pAttachments    = colorBlendAttachments.data(),
			.blendConstants  = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }}; // Optional

		// dynamic state
		const std::array<vk::DynamicState, 3> dynamicStates = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
			vk::DynamicState::eLineWidth
		};
		const vk::PipelineDynamicStateCreateInfo dynamicState{
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates    = dynamicStates.data()};

		const vk::raii::Device& device = m_Device->Get();

		// descriptor set layouts
		const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = vulkanShader->GetAllDescriptorSetLayouts();
		const std::vector<vk::PushConstantRange> pushConstantRanges = vulkanShader->GetPushConstantRanges();
		m_PipelineLayout = vk::raii::PipelineLayout(device, vk::PipelineLayoutCreateInfo{
			.setLayoutCount         = static_cast<uint32_t>(descriptorSetLayouts.size()),
			.pSetLayouts            = descriptorSetLayouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
			.pPushConstantRanges    = pushConstantRanges.data()});

		// Create pipeline (throws vk::SystemError on failure)
		m_GraphicsPipeline = vk::raii::Pipeline(device, nullptr, vk::GraphicsPipelineCreateInfo{
			.stageCount          = static_cast<uint32_t>(shaderStages.size()),
			.pStages             = shaderStages.data(),
			.pVertexInputState   = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState      = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState   = &multisampling,
			.pDepthStencilState  = &depthStencil, // Optional
			.pColorBlendState    = &colorBlending,
			.pDynamicState       = &dynamicState, // Optional
			.layout              = *m_PipelineLayout,
			.renderPass          = renderPass->GetHandle(),
			.subpass             = 0,
			.basePipelineHandle  = nullptr, // Optional
			.basePipelineIndex   = -1});    // Optional
	}

	void VulkanPipeline::Bind(const vk::raii::CommandBuffer& cmdBuffer, vk::PipelineBindPoint bindPoint) const
	{
		cmdBuffer.bindPipeline(bindPoint, *m_GraphicsPipeline);
	}

	vk::CullModeFlags VulkanPipeline::VulkanCullMode(CullMode cullMode)
	{
		switch (cullMode)
		{
		case CullMode::NONE: return vk::CullModeFlagBits::eNone;
		case CullMode::FRONT: return vk::CullModeFlagBits::eFront;
		case CullMode::BACK: return vk::CullModeFlagBits::eBack;
		}
		return vk::CullModeFlagBits::eNone;
	}
}
