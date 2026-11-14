//
//  VulkanBuffer.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2026/1/22.
//
#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"

namespace Ethane {

    // A buffer with its own dedicated memory allocation (one allocation per buffer, no VMA yet)
    class VulkanBuffer
    {
    public:
        explicit VulkanBuffer(const VulkanDevice* device);
        ~VulkanBuffer() { Destroy(); }

        bool CreateVulkanBuffer(uint32_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties, bool bindOnCreate);

        void Destroy();

        // Grows the buffer, keeping its contents. Usage must include TransferSrc | TransferDst.
        bool Resize(uint32_t newSize);

        void Bind(uint32_t offset);

        void* LockMemory(uint32_t offset, uint32_t size, vk::MemoryMapFlags flags = {});

        void UnlockMemory();

        // Host-visible buffers only: copies `size` bytes from `data + srcOffset` to `dstOffset` in this buffer
        void SetData(const void* data, uint32_t srcOffset, uint32_t size, uint32_t dstOffset);

        // Device-local buffers: uploads `size` bytes from `data` to `dstOffset` in this buffer via a staging buffer
        void TransferData(const void* data, uint32_t size, uint32_t dstOffset);

        // Records, submits and waits for a single buffer-to-buffer copy
        // A one-off copy on the graphics queue, not waited for (see the .cpp)
        static void CopyTo(
            const VulkanDevice* device,
            vk::Buffer source,
            uint32_t sourceOffset,
            vk::Buffer dest,
            uint32_t destOffset,
            uint32_t size);

        // Getter
        vk::Buffer GetHandle() const { return *m_Buffer; }

    protected:
        const VulkanDevice* m_Device = nullptr;
        VulkanLiveResource m_LiveResource;

        uint32_t m_Size = 0;
        vk::BufferUsageFlags m_Usage;
        vk::MemoryPropertyFlags m_MemoryProperties;
        uint32_t m_MemoryIndex = 0;

        // NOTE: declaration order matters, members are destroyed in reverse: buffer -> memory
        vk::raii::DeviceMemory m_Memory = nullptr;
        vk::raii::Buffer m_Buffer = nullptr;
    };

    class VulkanVertexBuffer : public IVertexBuffer, VulkanBuffer {
    public:
        VulkanVertexBuffer(const VulkanDevice* device, uint32_t size); // , VertexBufferUsage usage = VertexBufferUsage::Dynamic
        virtual ~VulkanVertexBuffer() = default;

        // `offset` is the destination offset in this buffer
        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

        using VulkanBuffer::GetHandle; // for binding
    };


    class VulkanIndexBuffer : public IIndexBuffer, VulkanBuffer {
    public:
        VulkanIndexBuffer(const VulkanDevice* device, uint32_t size); // size in byte
		virtual ~VulkanIndexBuffer() = default;

        // `offset` is the destination offset in this buffer
		void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		virtual uint32_t GetCount() const override { return m_Size / sizeof(uint32_t); }

		virtual uint32_t GetSize() const { return m_Size; }

        using VulkanBuffer::GetHandle; // for binding
    };

    // One host-visible copy per frame in flight: the GPU may still read frame N-1's copy while frame N's is written
    class VulkanUniformBuffer : public IUniformBuffer {
    public:
        VulkanUniformBuffer(const VulkanDevice* device, uint32_t size);
        virtual ~VulkanUniformBuffer() = default;

        // During a frame: writes the current frame's copy. Outside a frame: waits for the GPU, writes all copies.
        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
        virtual uint32_t GetSize() const override { return m_Size; }

        vk::Buffer GetHandle(uint32_t frameIndex) const { return m_Copies[frameIndex]->GetHandle(); }

    private:
        const VulkanDevice* m_Device = nullptr;
        uint32_t m_Size = 0;
        // unique_ptr: VulkanBuffer isn't movable
        std::array<std::unique_ptr<VulkanBuffer>, VulkanDevice::kMaxFramesInFlight> m_Copies;
    };
}
