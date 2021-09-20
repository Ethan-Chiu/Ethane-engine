#include "ethpch.h"
#include "VulkanVertexBuffer.h"

#include "VulkanContext.h"

namespace Ethane {

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size) // VertexBufferUsage usage
		: m_Size(size)
	{
		//Ref<VulkanVertexBuffer> instance = this;
		//Renderer::Submit([instance]() mutable
			{
				auto device = VulkanContext::GetDevice()->GetVulkanDevice();
				
				// VulkanAllocator allocator("VertexBuffer");
				CreateBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					m_VulkanBuffer, m_VertexBufferMemory);
			}
		// );
	}

	VulkanVertexBuffer::VulkanVertexBuffer(void* data, uint32_t size) //, VertexBufferUsage usage
		: m_Size(size)
	{
		// Ref<VulkanVertexBuffer> instance = this;
		// Renderer::Submit([instance]() mutable
			{
				auto device = VulkanContext::GetDevice()->GetVulkanDevice();

				// create staging buffer
				VkBuffer stagingBuffer;
				VkDeviceMemory stagingBufferMemory;
				CreateBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					stagingBuffer, stagingBufferMemory);
				
				// copy data to staging buffer
				void* dstData;
				vkMapMemory(device, stagingBufferMemory, 0, m_Size, 0, &dstData);
				memcpy(dstData, data, m_Size);
				vkUnmapMemory(device, stagingBufferMemory);
				
				// create vertex buffer (gpu local memory)
				CreateBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
					m_VulkanBuffer, m_VertexBufferMemory);

				// copy from staging buffer
				CopyBuffer(m_VulkanBuffer, stagingBuffer, m_Size);
				
				// cleanup staging buffer
				vkDestroyBuffer(device, stagingBuffer, nullptr);
				vkFreeMemory(device, stagingBufferMemory, nullptr);
			}
		// );
	}

	void VulkanVertexBuffer::Cleanup()
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		vkDestroyBuffer(device, m_VulkanBuffer, nullptr);
		vkFreeMemory(device, m_VertexBufferMemory, nullptr);
	}


	void VulkanVertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		// create staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// copy data to staging buffer
		void* dstData;
		vkMapMemory(device, stagingBufferMemory, 0, size, 0, &dstData);
		memcpy(dstData, (uint8_t*)data + offset, size);
		vkUnmapMemory(device, stagingBufferMemory);

		// copy from staging buffer
		CopyBuffer(m_VulkanBuffer, stagingBuffer, m_Size);

		// cleanup staging buffer
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	///////////////////////////////////////////////////////////////////////////
	// helper funtions ////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	void VulkanVertexBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
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
		allocInfo.memoryTypeIndex = Utils::FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void VulkanVertexBuffer::CopyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size) {
		// create command buffer
		VkCommandBuffer copyCmdBuffer = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);

		// record command
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(copyCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		// submit command buffer
		VulkanContext::GetDevice()->SubmitCommandBuffer(copyCmdBuffer);
	}
}