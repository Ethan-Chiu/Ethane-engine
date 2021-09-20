#pragma once

#include "Ethane/Renderer/VertexBuffer.h"

#include "Vulkan.h"

namespace Ethane {

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer() {}
		VulkanVertexBuffer(uint32_t size); // , VertexBufferUsage usage = VertexBufferUsage::Dynamic
		VulkanVertexBuffer(void* data, uint32_t size); // , VertexBufferUsage usage = VertexBufferUsage::Static

		void Cleanup();

		// temp
		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual void SetData(const void* data, uint32_t size) override { SetData(data, size, 0); };

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0);
		
		virtual void SetLayout(const VertexBufferLayout& layout) override {} // test
	
		// virtual unsigned int GetSize() const override { return m_Size; }
		// virtual RendererID GetRendererID() const override { return 0; }

		// Getter
		VkBuffer GetVulkanBuffer() const { return m_VulkanBuffer; }
		virtual const VertexBufferLayout& GetLayout() const override { return VertexBufferLayout(); } // test

	private: 
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		// uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void CopyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);
	private:
		uint32_t m_Size = 0;
		// Buffer m_LocalData;

		VkBuffer m_VulkanBuffer = nullptr;
		VkDeviceMemory m_VertexBufferMemory = nullptr;
		// VmaAllocation m_MemoryAllocation;
	};

}