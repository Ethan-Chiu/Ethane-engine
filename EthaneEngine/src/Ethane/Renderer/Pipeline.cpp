#include "ethpch.h"
#include "Pipeline.h"

#include "Renderer.h"

#include "Platform/OpenGL/OpenGLPipeline.h"

namespace Ethane {

	Ref<Pipeline> Create(const PipelineSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLPipeline>(spec);
		}

		ETH_CORE_ASSERT(false, "Unknpwn RendererAPI");
		return nullptr;
	}
}