#include "ethpch.h"
#include "IndexBuffer.h"

#include "Renderer.h"

#include "Ethane/Platform/OpenGL/OpenGLIndexBuffer.h"
#include "Ethane/Platform/Vulkan/VulkanIndexBuffer.h"

namespace Ethane {

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t size)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:
			ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLIndexBuffer>(indices, size/sizeof(uint32_t));
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanIndexBuffer>(indices, size);
		}

		ETH_CORE_ASSERT(false, "Unknpwn RendererAPI");
		return nullptr;
	}

}