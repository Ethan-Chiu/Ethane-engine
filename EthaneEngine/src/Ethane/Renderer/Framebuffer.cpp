#include "ethpch.h"
#include "Framebuffer.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"
#include "Platform/Vulkan/vulkanFramebuffer.h"

namespace Ethane {

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:
			ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLFramebuffer>(spec);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanFramebuffer>(spec);
		}

		ETH_CORE_ASSERT(false, "Unknpwn RendererAPI");
		return nullptr;
	}

}