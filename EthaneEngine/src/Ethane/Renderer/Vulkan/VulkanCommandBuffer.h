#pragma once
#include "vulkan/vulkan.h"

namespace Ethane {

    enum class QueueFamilyTypes;

    class VulkanCommandBuffer
    {
    public:
        enum class CommandBufferState {
            READY,
            RECORDING,
            IN_RENDER_PASS,
            RECORDING_ENDED,
            SUBMITTED,
            NOT_ALLOCATED
        };

        VulkanCommandBuffer();

        void Allocate(VkCommandPool pool, bool isPrimary);
        void Free(VkCommandPool pool);

        void Begin(bool isOneTimeUse, bool isRenderpassContinue, bool isSimultaneousUse);
        void End();

        void UpdateSubmitted();

        void Reset();

        void AllocateAndBeginSingleUse(VkCommandPool pool);
        void EndSingleUse(VkCommandPool pool, VkQueue queue);

        void AllocateAndBeginSingleUse(QueueFamilyTypes type);
        void EndSingleUse(QueueFamilyTypes type);

        VkCommandBuffer GetHandle() { return m_CommandBuffer; };

    private:
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        CommandBufferState m_State;

        VkDevice m_Device;
    };
}