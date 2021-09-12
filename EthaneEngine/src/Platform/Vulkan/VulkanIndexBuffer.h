#pragma once

#include "Ethane/Renderer/IndexBuffer.h"

#include "Vulkan.h"

namespace Ethane {

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(uint32_t size);
		VulkanIndexBuffer(void* data, uint32_t size = 0);
		virtual ~VulkanIndexBuffer();

		void Cleanup();

		void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind() const override {};
		virtual void Unbind() const override {};


		// virtual RendererID GetRendererID() const;

		//Getter
		virtual uint32_t GetCount() const override { return m_Size / sizeof(uint32_t); }
		virtual uint32_t GetSize() const { return m_Size; }
		VkBuffer GetVulkanBuffer() { return m_VulkanBuffer; }

	private:
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		// uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void CopyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);

	private:
		uint32_t m_Size = 0;

		VkBuffer m_VulkanBuffer = nullptr;
		VkDeviceMemory m_IndexBufferMemory = nullptr;
		// Buffer m_LocalData;
	};

}