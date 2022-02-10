#include "ethpch.h"
#include "VulkanIndexBuffer.h"

#include "VulkanContext.h"

// #include "Ethane/Renderer/Renderer.h"

namespace Ethane {

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size) // size in byte
		: m_Size(size)
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		Utils::CreateBuffer(m_Size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_VulkanBuffer, m_IndexBufferMemory);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(void* data, uint32_t size)
		: m_Size(size)
	{
		// Ref<VulkanIndexBuffer> instance = this;
		// Renderer::Submit([instance]() mutable
		{
			auto device = VulkanContext::GetDevice()->GetVulkanDevice();

			// create staging buffer
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			Utils::CreateBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			// copy data to staging buffer
			void* dstData;
			vkMapMemory(device, stagingBufferMemory, 0, m_Size, 0, &dstData);
			memcpy(dstData, data, m_Size);
			vkUnmapMemory(device, stagingBufferMemory);

			// create index buffer (gpu local memory)
			Utils::CreateBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VulkanBuffer, m_IndexBufferMemory);

			// copy from staging buffer
			Utils::CopyBuffer(m_VulkanBuffer, stagingBuffer, m_Size);

			// cleanup staging buffer
			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingBufferMemory, nullptr);
		}
			// );
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
	}

	void VulkanIndexBuffer::Cleanup()
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		vkDestroyBuffer(device, m_VulkanBuffer, nullptr);
		vkFreeMemory(device, m_IndexBufferMemory, nullptr);
	}

	void VulkanIndexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		// create staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Utils::CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// copy data to staging buffer
		void* dstData;
		vkMapMemory(device, stagingBufferMemory, 0, size, 0, &dstData);
		memcpy(dstData, (uint8_t*)data + offset, size);
		vkUnmapMemory(device, stagingBufferMemory);

		// copy from staging buffer
		Utils::CopyBuffer(m_VulkanBuffer, stagingBuffer, m_Size);

		// cleanup staging buffer
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

}
