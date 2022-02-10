#include "ethpch.h"
#include "RenderCommandBuffer.h"

#include "Ethane/Platform/Vulkan/VulkanRenderCommandBuffer.h"

#include "RendererAPI.h"

namespace Ethane {

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create(uint32_t count, const std::string& debugName, bool fromSwapChain)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return CreateRef<VulkanRenderCommandBuffer>(count, debugName, fromSwapChain);
		}
		ETH_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}