#include "ethpch.h"
#include "VertexBuffer.h"

#include "RendererAPI.h"

#include "Platform/OpenGL/OpenGLVertexBuffer.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"

namespace Ethane {

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: 
				ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
				return nullptr;
			case RendererAPI::API::OpenGL:
				return CreateRef<OpenGLVertexBuffer>(size);
		}

		ETH_CORE_ASSERT(false, "Unknpwn RendererAPI");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(void* vertices, uint32_t size)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:
			ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLVertexBuffer>(vertices, size);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanVertexBuffer>(vertices, size);
		}

		ETH_CORE_ASSERT(false, "Unknpwn RendererAPI");
		return nullptr;
	}

}