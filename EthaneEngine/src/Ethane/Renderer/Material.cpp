#include "ethpch.h"
#include "Material.h"

// #include "Ethane/Platform/OpenGL/OpenGLMaterial.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include "RendererAPI.h"

namespace Ethane {

	Ref<Material> Material::Create(const Ref<Shader>& shader, const std::string& name)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::Vulkan: return CreateRef<VulkanMaterial>(shader, name);
		// case RendererAPI::API::OpenGL: return Ref<OpenGLMaterial>::Create(shader, name);
		}
		ETH_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	// Ref<Material> Material::Copy(const Ref<Material>& other, const std::string& name)
	// {
	// 	switch (RendererAPI::GetAPI())
	// 	{
	// 	case RendererAPIType::None: return nullptr;
	// 	case RendererAPIType::Vulkan: return Ref<VulkanMaterial>::Create(other, name);
	// 	}
	// 	ETH_CORE_ASSERT(false, "Unknown RendererAPI");
	// 	return nullptr;
	// }

}