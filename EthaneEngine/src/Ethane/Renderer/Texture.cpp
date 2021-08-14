#include "ethpch.h"
#include "Texture.h"

#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Ethane {

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:
			ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture2D>(width, height);
		}

		ETH_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:
			ETH_CORE_ASSERT(false, "endererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture2D>(path);
		}

		ETH_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}