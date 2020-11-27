#include "ethpch.h"
#include "VertexArray.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Ethane {

	VertexArray* VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::None:
			ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::OpenGL:
			return new OpenGLVertexArray();
		}

		ETH_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}