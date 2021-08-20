#pragma once

#include "Vulkan.h"

namespace Ethane {

	class VulkanUniformBuffer
	{
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		void Cleanup();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0);

		const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return m_DescriptorInfo; }
	private:
		void Invalidate();
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	private:
		VkBuffer m_VulkanBuffer;
		VkDeviceMemory m_UniformBufferMemory = nullptr;

		VkDescriptorBufferInfo m_DescriptorInfo{};

		uint32_t m_Size = 0;
		uint32_t m_Binding = 0;
		std::string m_Name;
		VkShaderStageFlagBits m_ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	};

}