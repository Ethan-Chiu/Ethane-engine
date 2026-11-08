#include "ethpch.h"
#include "VulkanUtils.h"
#include "VulkanCommandBuffer.h"


namespace Ethane {

    VulkanCommandBuffer::VulkanCommandBuffer(const VulkanDevice* device)
        : m_Device(device)
    {
    }

    void VulkanCommandBuffer::Allocate(const vk::raii::CommandPool& pool, bool isPrimary)
    {
        m_State = CommandBufferState::NOT_ALLOCATED;

        vk::raii::CommandBuffers buffers(m_Device->Get(), vk::CommandBufferAllocateInfo{
            .commandPool        = *pool,
            .level              = isPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary,
            .commandBufferCount = 1});
        m_CommandBuffer = std::move(buffers.front());

        m_State = CommandBufferState::READY;
    }

    void VulkanCommandBuffer::Free()
    {
        m_CommandBuffer = nullptr; // freed back to the pool it was allocated from
        m_State = CommandBufferState::NOT_ALLOCATED;
    }

    void VulkanCommandBuffer::Begin(bool isOneTimeUse, bool isRenderpassContinue, bool isSimultaneousUse)
    {
        using Usage = vk::CommandBufferUsageFlagBits;
        vk::CommandBufferUsageFlags flags;
        if (isOneTimeUse) {
            flags |= Usage::eOneTimeSubmit;
        }
        if (isRenderpassContinue) {
            flags |= Usage::eRenderPassContinue;
        }
        if (isSimultaneousUse) {
            flags |= Usage::eSimultaneousUse;
        }

        m_CommandBuffer.begin(vk::CommandBufferBeginInfo{ .flags = flags });
        m_State = CommandBufferState::RECORDING;
    }

    void VulkanCommandBuffer::End() {
        m_CommandBuffer.end();
        m_State = CommandBufferState::RECORDING_ENDED;
    }

    void VulkanCommandBuffer::UpdateSubmitted()
    {
        m_State = CommandBufferState::SUBMITTED;
    }

    void VulkanCommandBuffer::Reset() {
        m_State = CommandBufferState::READY;
    }

    void VulkanCommandBuffer::AllocateAndBeginSingleUse(const vk::raii::CommandPool& pool)
    {
        Allocate(pool, true);
        Begin(true, false, false);
    }

    void VulkanCommandBuffer::EndSingleUse(const vk::raii::Queue& queue)
    {
        End();

        // Submit the queue
        const vk::CommandBuffer handle = *m_CommandBuffer;
        queue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &handle });

        // Wait for it to finish
        queue.waitIdle();

        // Free the command buffer.
        Free();
    }

    void VulkanCommandBuffer::AllocateAndBeginSingleUse(QueueFamilyTypes type)
    {
        AllocateAndBeginSingleUse(m_Device->GetCommandPool(type));
    }

    void VulkanCommandBuffer::EndSingleUse(QueueFamilyTypes type)
    {
        EndSingleUse(m_Device->GetQueue(type));
    }

}
