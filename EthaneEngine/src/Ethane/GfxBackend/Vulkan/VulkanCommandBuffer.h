#pragma once
#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"

namespace Ethane {

    class VulkanCommandBuffer : public ICommandList
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

        explicit VulkanCommandBuffer(const VulkanDevice* device);

        // Allocates from `pool`; the buffer is freed back to it by Free() or on destruction
        void Allocate(const vk::raii::CommandPool& pool, bool isPrimary);
        void Free();

        void Begin(bool isOneTimeUse, bool isRenderpassContinue, bool isSimultaneousUse);
        void End();

        void UpdateSubmitted();

        void Reset();

        void AllocateAndBeginSingleUse(const vk::raii::CommandPool& pool);
        // Ends, submits to `queue`, waits for the queue to be idle, then frees the buffer
        void EndSingleUse(const vk::raii::Queue& queue);

        void AllocateAndBeginSingleUse(QueueFamilyTypes type);
        void EndSingleUse(QueueFamilyTypes type);

        // ICommandList: recording (the buffer must be in the RECORDING state)
        virtual void BindPipeline(const IPipeline& pipeline) override;
        virtual void BindVertexBuffer(const IVertexBuffer& buffer) override;
        virtual void BindIndexBuffer(const IIndexBuffer& buffer) override;
        virtual void BindBindingSet(IBindingSet& bindingSet) override;
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;

        const vk::raii::CommandBuffer& Get() const { return m_CommandBuffer; }
        vk::CommandBuffer GetHandle() const { return *m_CommandBuffer; }

    private:
        const VulkanDevice* m_Device = nullptr;
        VulkanLiveResource m_LiveResource;

        vk::raii::CommandBuffer m_CommandBuffer = nullptr;
        CommandBufferState m_State = CommandBufferState::NOT_ALLOCATED;

        // Last pipeline bound while recording: descriptor sets are bound against its layout
        vk::PipelineLayout m_BoundPipelineLayout = nullptr;
    };
}
