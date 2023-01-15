#include "ethpch.h"
#include "RenderCommand.h"

#include "Vulkan/VulkanRendererAPI.h"

namespace Ethane {

	Ref<RendererAPI> RenderCommand::s_RendererAPI = CreateRef<VulkanRendererAPI>();

	void RenderCommand::Init()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:
			ETH_CORE_ASSERT("endererAPI::None is currently not supported!");
			break;
		case RendererAPI::API::Vulkan:
			s_RendererAPI = CreateRef<VulkanRendererAPI>();
			ETH_CORE_INFO("Vulkan graphic API selected");
			break;
		}
		s_RendererAPI->Init();
	}

}