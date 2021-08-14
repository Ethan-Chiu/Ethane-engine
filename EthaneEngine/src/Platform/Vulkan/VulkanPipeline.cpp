#include "ethpch.h"
#include "VulkanPipeline.h"

#include "VulkanContext.h"

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
			}
			ETH_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}
	}

	VulkanPipeline::VulkanPipeline(VkRenderPass renderPass, VertexBufferLayout layout)
		:m_RenderPass(renderPass), m_Layout(layout)
	{
		Invalidate();
	}

	VulkanPipeline::~VulkanPipeline()
	{
	}

	void VulkanPipeline::Cleanup()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		m_VulkanShader->Cleanup();
		vkDestroyPipeline(device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	}

	void VulkanPipeline::Invalidate()
	{
		// Shader Stage
		// TODO: remove this
		m_VulkanShader = CreateRef<VulkanShader>( "assets/shaders/test.glsl" );
		const auto& shaderStages = m_VulkanShader->GetPipelineShaderStageCreateInfos();

		// Fixed Function 
		// vertex input
		// TODO:
		VkVertexInputBindingDescription vertexInputBinding = {};
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = m_Layout.GetStride();
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(m_Layout.GetElementCount());
		uint32_t location = 0;
		for (auto element : m_Layout)
		{
			attributeDescriptions[location].binding = 0;
			attributeDescriptions[location].location = location;
			attributeDescriptions[location].format = Utils::ShaderDataTypeToVulkanFormat(element.Type);
			attributeDescriptions[location].offset = element.Offset;
			
			location++;
		}
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// viewport
		// VkViewport viewport{};
		// viewport.x = 0.0f;
		// viewport.y = 0.0f;
		// viewport.width = (float)swapChainExtent.width;
		// viewport.height = (float)swapChainExtent.height;
		// viewport.minDepth = 0.0f;
		// viewport.maxDepth = 1.0f;
		// VkRect2D scissor{};
		// scissor.offset = { 0, 0 };
		// scissor.extent = swapChainExtent;
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		// depth and stencil testing

		// color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		// dynamic state
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;


		// Create pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState; // Optional

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		// TODO: remove this
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
		pipelineInfo.layout = m_PipelineLayout;

		// TODO: 
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}
}