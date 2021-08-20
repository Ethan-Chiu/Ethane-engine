#include "ethpch.h"
#include "VulkanUniformBuffer.h"

#include "VulkanContext.h"

namespace Ethane {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		Invalidate();
	}

	void VulkanUniformBuffer::Cleanup()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		vkDestroyBuffer(device, m_VulkanBuffer, nullptr);
		vkFreeMemory(device, m_UniformBufferMemory, nullptr);
	}

	void VulkanUniformBuffer::Invalidate()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		CreateBuffer(m_Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VulkanBuffer, m_UniformBufferMemory);

		m_DescriptorInfo.buffer = m_VulkanBuffer;
		m_DescriptorInfo.offset = 0;
		m_DescriptorInfo.range = m_Size;
	}	

	void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		void* dstdata;
		vkMapMemory(device, m_UniformBufferMemory, 0, m_Size, 0, &dstdata);
		memcpy(dstdata, (const uint8_t*)data + offset, size);
		vkUnmapMemory(device, m_UniformBufferMemory);
	}


	void VulkanUniformBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	uint32_t VulkanUniformBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		auto physicalDevice = VulkanContext::GetPhysicalDevice()->GetVulkanPhysicalDevice();

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
	}
}