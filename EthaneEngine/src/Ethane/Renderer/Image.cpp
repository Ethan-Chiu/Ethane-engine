#include "ethpch.h"
#include "Image.h"

// #include "Platform/OpenGL/OpenGLImage.h"
#include "Platform/Vulkan/VulkanImage.h"

#include "Ethane/Renderer/RendererAPI.h"

namespace Ethane{

	// Ref<Image2D> Image2D::Create(ImageSpecification specification, Buffer buffer)
	// {
	// 	switch (RendererAPI::GetAPI())
	// 	{
	// 	case RendererAPI::API::None: return nullptr;
	// 	// case RendererAPI::API::OpenGL: return Ref<OpenGLImage2D>::Create(specification, buffer);
	// 	case RendererAPI::API::Vulkan: return Ref<VulkanImage2D>::Create(specification);
	// 	}
	// 	ETH_CORE_ASSERT(false, "Unknown RendererAPI");
	// 	return nullptr;
	// }

	Ref<Image2D> Image2D::Create(ImageSpecification specification, void* data)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		// case RendererAPI::API::OpenGL: return Ref<OpenGLImage2D>::Create(specification, data);
		case RendererAPI::API::Vulkan: return CreateRef<VulkanImage2D>(specification);
		}
		ETH_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}