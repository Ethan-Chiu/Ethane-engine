#pragma once

#include "Ethane/Renderer/RenderCommandBuffer.h"
#include "vulkan/vulkan.h"

namespace Ethane {

	class VulkanRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		VulkanRenderCommandBuffer(uint32_t count = 0, std::string debugName = "");
		~VulkanRenderCommandBuffer() override;

		virtual void Begin() override;
		virtual void End() override;
		virtual void Submit() override;

		// virtual uint64_t BeginTimestampQuery() override;
		// virtual void EndTimestampQuery(uint64_t queryID) override;

		// Getter
		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
		{
			ETH_CORE_ASSERT(frameIndex < m_CommandBuffers.size());
			return m_CommandBuffers[frameIndex];
		}

	private:
		std::string m_DebugName;

		VkCommandPool m_CommandPool = nullptr;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		std::vector<VkFence> m_WaitFences;
	};

}
