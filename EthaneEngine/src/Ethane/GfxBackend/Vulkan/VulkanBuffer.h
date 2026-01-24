//
//  VulkanBuffer.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2026/1/22.
//

#include <vulkan/vulkan.h>
#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"

namespace Ethane {

    class VulkanBuffer
    {
    public:
        VulkanBuffer(const VulkanDevice* device);
        ~VulkanBuffer() = default;
        
        bool CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, uint32_t memory_property_flags, bool bind_on_create);

        void Destroy();

        bool Resize(uint32_t new_size, VkQueue queue, VkCommandPool pool);

        void Bind(uint32_t offset);

        void* LockMemory(uint32_t offset, uint32_t size, uint32_t flags);

        void UnlockMemory();

        void SetData(const void* data, uint32_t srcOffset, uint32_t size, uint32_t dstOffset, uint32_t flags);

        void TransferData(const void* data, uint32_t size, uint32_t offset);

        static void CopyTo(
            const VulkanDevice* device,
            VkCommandPool pool,
            VkFence fence,
            VkQueue queue,
            VkBuffer source,
            uint32_t source_offset,
            VkBuffer dest,
            uint32_t dest_offset,
            uint32_t size);

        // Getter
        VkBuffer GetHandle() { return m_Buffer; }

    protected:
        const VulkanDevice* m_Device = nullptr;
        
        uint32_t m_Size = 0;
        VkBufferUsageFlags m_Usage;
        
        VkBuffer m_Buffer = nullptr;

        uint32_t m_MemoryFlag;
        uint32_t m_MemoryIndex;
        VkDeviceMemory m_Memory = nullptr;
    
    };

    class VulkanVertexBuffer : public IVertexBuffer, VulkanBuffer {
    public: 
        VulkanVertexBuffer(const VulkanDevice* device, uint32_t size); // , VertexBufferUsage usage = VertexBufferUsage::Dynamic
        virtual ~VulkanVertexBuffer();

        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
    };


    class VulkanIndexBuffer : public IIndexBuffer, VulkanBuffer {
    public: 
        VulkanIndexBuffer(const VulkanDevice* device, uint32_t size); // size in byte
		virtual ~VulkanIndexBuffer();

		void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		virtual uint32_t GetCount() const override { return m_Size / sizeof(uint32_t); }

		virtual uint32_t GetSize() const { return m_Size; }
    };
}