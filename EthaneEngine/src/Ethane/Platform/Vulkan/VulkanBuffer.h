#pragma once

#include "Vulkan.h"

namespace Ethane {

    class VulkanBuffer
    {
    public:
        bool CreateVulkanBuffer(uint32_t size, VkBufferUsageFlagBits usage, uint32_t memory_property_flags, bool bind_on_create);

        void Destroy();

        bool Resize(uint32_t new_size, VkQueue queue, VkCommandPool pool);

        void Bind(uint32_t offset);

        void* LockMemory(uint32_t offset, uint32_t size, uint32_t flags);

        void UnlockMemory();

        void SetData(const void* data, uint32_t size, uint32_t offset, uint32_t flags);

        static void CopyTo(
            VkCommandPool pool,
            VkFence fence,
            VkQueue queue,
            VkBuffer source,
            uint32_t source_offset,
            VkBuffer dest,
            uint32_t dest_offset,
            uint32_t size);

    private:
        uint32_t m_Size = 0;
        VkBufferUsageFlagBits m_Usage;
        
        VkBuffer m_Buffer = nullptr;

        uint32_t m_MemoryFlag;
        uint32_t m_MemoryIndex;
        VkDeviceMemory m_Memory = nullptr;
    
    };

}