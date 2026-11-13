//
//  VulkanBuffer.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2026/1/22.
//

#include "ethpch.h"
#include "VulkanBuffer.h"
#include "VulkanUtils.h"
#include "VulkanCommandBuffer.h"


namespace Ethane {

    VulkanBuffer::VulkanBuffer(const VulkanDevice* device)
        :m_Device(device)
    {
    }

    bool VulkanBuffer::CreateVulkanBuffer(
        uint32_t size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags memoryProperties,
        bool bindOnCreate) {

        const vk::raii::Device& device = m_Device->Get();

        m_Size = size;
        m_Usage = usage;
        m_MemoryProperties = memoryProperties;

        vk::raii::Buffer buffer(device, vk::BufferCreateInfo{
            .size        = size,
            .usage       = usage,
            .sharingMode = vk::SharingMode::eExclusive});

        // allocate memory
        const vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        m_MemoryIndex = m_Device->FindMemoryType(memRequirements.memoryTypeBits, static_cast<VkMemoryPropertyFlags>(memoryProperties)); // TODO: handle can't find

        try {
            m_Memory = vk::raii::DeviceMemory(device, vk::MemoryAllocateInfo{
                .allocationSize  = memRequirements.size,
                .memoryTypeIndex = m_MemoryIndex});
        } catch (const vk::SystemError& err) {
            ETH_CORE_ERROR("failed to allocate buffer memory! {0}", err.what());
            return false; // `buffer` is destroyed on return
        }
        m_Buffer = std::move(buffer);

        if (bindOnCreate) {
            m_Buffer.bindMemory(*m_Memory, 0);
        }
        return true;
    }

    void VulkanBuffer::Destroy() {
        // Frames in flight may still read it: destroyed (buffer, then memory) once they've finished
        if (*m_Buffer || *m_Memory) {
            m_Device->Retire(std::move(m_Buffer), std::move(m_Memory));
        }
        m_Buffer = nullptr;
        m_Memory = nullptr;
        m_Size = 0;
        // buffer->is_locked = false;
    }

    bool VulkanBuffer::Resize(uint32_t newSize) {

        // Sanity check.
        if (newSize < m_Size) {
            ETH_CORE_ERROR("vulkan_buffer_resize requires that new size be larger than the old. Not doing this could lead to data loss.");
            return false;
        }
        ETH_CORE_ASSERT((m_Usage & vk::BufferUsageFlagBits::eTransferSrc) && (m_Usage & vk::BufferUsageFlagBits::eTransferDst),
                        "Resize copies the old contents: the buffer needs TransferSrc and TransferDst usage");

        const vk::raii::Device& device = m_Device->Get();
        const uint32_t oldSize = m_Size;

        // Create new buffer + memory
        vk::raii::Buffer newBuffer(device, vk::BufferCreateInfo{
            .size        = newSize,
            .usage       = m_Usage,
            .sharingMode = vk::SharingMode::eExclusive});

        const vk::MemoryRequirements memRequirements = newBuffer.getMemoryRequirements();
        vk::raii::DeviceMemory newMemory = nullptr;
        try {
            newMemory = vk::raii::DeviceMemory(device, vk::MemoryAllocateInfo{
                .allocationSize  = memRequirements.size,
                .memoryTypeIndex = m_MemoryIndex});
        } catch (const vk::SystemError& err) {
            ETH_CORE_ERROR("failed to allocate buffer memory during resize! {0}", err.what());
            return false;
        }
        newBuffer.bindMemory(*newMemory, 0);

        // Copy over the old contents (only the old size exists in the old buffer)
        CopyTo(m_Device, m_Device->GetGraphicsCommandPool(), m_Device->GetGraphicsQueue(), *m_Buffer, 0, *newBuffer, 0, oldSize);

        // Frames in flight may still read the old buffer: retired, destroyed once they've finished
        m_Device->Retire(std::move(m_Buffer), std::move(m_Memory));
        m_Memory = std::move(newMemory);
        m_Buffer = std::move(newBuffer);
        m_Size = newSize;

        return true;
    }

    void VulkanBuffer::Bind(uint32_t offset) {
        m_Buffer.bindMemory(*m_Memory, offset);
    }

    void* VulkanBuffer::LockMemory(uint32_t offset, uint32_t size, vk::MemoryMapFlags flags) {
        return m_Memory.mapMemory(offset, size, flags);
    }

    void VulkanBuffer::UnlockMemory() {
        m_Memory.unmapMemory();
    }

    void VulkanBuffer::SetData(const void* data, uint32_t srcOffset, uint32_t size, uint32_t dstOffset) {
        ETH_CORE_ASSERT(m_MemoryProperties & vk::MemoryPropertyFlagBits::eHostVisible, "SetData needs host-visible memory, use TransferData");
        ETH_CORE_ASSERT(dstOffset + size <= m_Size, "SetData writes past the end of the buffer");

        void* dstData = m_Memory.mapMemory(dstOffset, size);
        memcpy(dstData, static_cast<const uint8_t*>(data) + srcOffset, size);
        m_Memory.unmapMemory();
    }

    void VulkanBuffer::TransferData(const void* data, uint32_t size, uint32_t dstOffset) {
        ETH_CORE_ASSERT(m_Usage & vk::BufferUsageFlagBits::eTransferDst, "TransferData needs TransferDst usage");
        ETH_CORE_ASSERT(dstOffset + size <= m_Size, "TransferData writes past the end of the buffer");

        // create staging buffer (destroyed at the end of this scope)
        VulkanBuffer stagingBuffer{m_Device};
        stagingBuffer.CreateVulkanBuffer(size, vk::BufferUsageFlagBits::eTransferSrc,
                                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, true);

        // copy data to staging buffer
        stagingBuffer.SetData(data, 0, size, 0);

        // copy from staging buffer to `dstOffset` in this buffer
        VulkanBuffer::CopyTo(m_Device, m_Device->GetGraphicsCommandPool(), m_Device->GetGraphicsQueue(), stagingBuffer.GetHandle(), 0, *m_Buffer, dstOffset, size);
    }

    void VulkanBuffer::CopyTo(
        const VulkanDevice* device,
        const vk::raii::CommandPool& pool,
        const vk::raii::Queue& queue,
        vk::Buffer source,
        uint32_t sourceOffset,
        vk::Buffer dest,
        uint32_t destOffset,
        uint32_t size) {

        // create command buffer
        VulkanCommandBuffer tempCommandBuf{device};
        tempCommandBuf.AllocateAndBeginSingleUse(pool);

        // record command
        tempCommandBuf.Get().copyBuffer(source, dest, vk::BufferCopy{
            .srcOffset = sourceOffset,
            .dstOffset = destOffset,
            .size      = size});

        // submit command buffer and wait for complete
        tempCommandBuf.EndSingleUse(queue);
    }


    // Vertex/index buffers are device-local and filled through staging copies.
    // TransferSrc is included too so Resize() can copy the old contents out.
    static constexpr vk::BufferUsageFlags kTransferUsage = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;

    VulkanVertexBuffer::VulkanVertexBuffer(const VulkanDevice* device, uint32_t size)
        :VulkanBuffer(device)
	{
		CreateVulkanBuffer(size, kTransferUsage | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, true);
	}

	void VulkanVertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		TransferData(data, size, offset);
	}


    VulkanIndexBuffer::VulkanIndexBuffer(const VulkanDevice* device, uint32_t size)
        :VulkanBuffer(device)
	{
		CreateVulkanBuffer(size, kTransferUsage | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, true);
	}

	void VulkanIndexBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		TransferData(data, size, offset);
	}


    VulkanUniformBuffer::VulkanUniformBuffer(const VulkanDevice* device, uint32_t size)
        : m_Device(device), m_Size(size)
    {
        for (auto& copy : m_Copies) {
            copy = std::make_unique<VulkanBuffer>(device);
            copy->CreateVulkanBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
                                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, true);
        }
    }

    void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
    {
        if (m_Device->IsFrameInProgress()) {
            // the window target waited for this frame slot's fence, so the GPU is done with its copy
            m_Copies[m_Device->GetFrameIndex()]->SetData(data, 0, size, offset);
            return;
        }

        // Outside a frame any copy may still be in use by a submitted frame
        m_Device->Get().waitIdle();
        for (auto& copy : m_Copies) {
            copy->SetData(data, 0, size, offset);
        }
    }
}
