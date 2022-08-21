#pragma once

#include "Ethane/Core/Base.h"
#include "VulkanUniformBuffer.h"
#include <map>

namespace Ethane {

	class VulkanUniformBufferSet // : public UniformBufferSet
	{
	public:
		VulkanUniformBufferSet(uint32_t frames)
			: m_Frames(frames) {}
		virtual ~VulkanUniformBufferSet() {}

		virtual void Create(uint32_t size, uint32_t binding) 
		{
			for (uint32_t frame = 0; frame < m_Frames; frame++)
			{
				Ref<VulkanUniformBuffer> uniformBuffer = CreateRef<VulkanUniformBuffer>(size, binding);
				Set(uniformBuffer, frame, 0);
			}
		}

		virtual Ref<VulkanUniformBuffer> Get(uint32_t frame, uint32_t set, uint32_t binding)
		{
			ETH_CORE_ASSERT(m_UniformBuffers.find(frame) != m_UniformBuffers.end());
			ETH_CORE_ASSERT(m_UniformBuffers.at(frame).find(set) != m_UniformBuffers.at(frame).end());
			ETH_CORE_ASSERT(m_UniformBuffers.at(frame).at(set).find(binding) != m_UniformBuffers.at(frame).at(set).end());

			return m_UniformBuffers.at(frame).at(set).at(binding);
		}

		virtual void Set(Ref<VulkanUniformBuffer> uniformBuffer, uint32_t frame = 0, uint32_t set = 0)
		{
			m_UniformBuffers[frame][set][uniformBuffer->GetBinding()] = uniformBuffer;
		}
	private:
		uint32_t m_Frames;
		std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, Ref<VulkanUniformBuffer>>>> m_UniformBuffers; // frame->set->binding
	};
}