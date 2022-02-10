#include "ethpch.h"
#include "UniformBuffer.h"

#include "RendererAPI.h"
#include "Ethane/Platform/OpenGL/OpenGLUniformBuffer.h"

namespace Ethane {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:    ETH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLUniformBuffer>(size, binding);
		}

		ETH_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}