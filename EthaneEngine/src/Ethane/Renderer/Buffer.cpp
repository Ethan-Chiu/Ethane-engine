#include "ethpch.h"
#include "Buffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Ethane {

	VertexBuffer* VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::None: 
				ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
				return nullptr;
			case RendererAPI::OpenGL:
				return new OpenGLVertexBuffer(vertices, size);
		}

		ETH_CORE_ASSERT(false, "Unknpwn RendererAPI");
		return nullptr;
	}

	IndexBuffer* IndexBuffer::Create(uint32_t* indices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::None:
				ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
				return nullptr;
			case RendererAPI::OpenGL:
				return new OpenGLIndexBuffer(indices, size);
		}

		ETH_CORE_ASSERT(false, "Unknpwn RendererAPI");
		return nullptr;
	}

}