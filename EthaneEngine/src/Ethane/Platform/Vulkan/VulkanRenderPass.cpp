#include "ethpch.h"
#include "VulkanRenderPass.h"

#include "VulkanFramebuffer.h"

namespace Ethane {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
		m_RenderPass = std::dynamic_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer)->GetRenderPass();
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
	}

}