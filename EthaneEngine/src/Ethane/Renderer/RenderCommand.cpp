#include "ethpch.h"
#include "RenderCommand.h"

#include "Ethane/Platform/OpenGL/OpenGLRendererAPI.h"
#include "Ethane/Platform/Vulkan/VulkanRendererAPI.h"

namespace Ethane {

	Ref<RendererAPI> RenderCommand::s_RendererAPI = CreateRef<OpenGLRendererAPI>();

	void RenderCommand::Init()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:
			ETH_CORE_ASSERT("endererAPI::None is currently not supported!");
			break;
		case RendererAPI::API::OpenGL:
			s_RendererAPI = CreateRef<OpenGLRendererAPI>();
			ETH_CORE_INFO("OpenGL graphic API selected");
			break;
		case RendererAPI::API::Vulkan:
			s_RendererAPI = CreateRef<VulkanRendererAPI>();
			ETH_CORE_INFO("Vulkan graphic API selected");
			break;
		}
		s_RendererAPI->Init();
	}

}