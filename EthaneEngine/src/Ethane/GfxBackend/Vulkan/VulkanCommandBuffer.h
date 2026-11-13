#pragma once
#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"

namespace Ethane { class VulkanPipeline; class VulkanRenderPass; }

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
        // Ends, submits to `queue`, waits for this command buffer to finish, then frees it
        void EndSingleUse(const vk::raii::Queue& queue);

        void AllocateAndBeginSingleUse(QueueFamilyTypes type);
        void EndSingleUse(QueueFamilyTypes type);
        // Submits without waiting: for work nothing on the CPU waits for (later submissions on the queue are
        // ordered after its barriers). The command buffer is freed through the device's deletion queue.
        void EndSingleUseAsync(QueueFamilyTypes type);

        // Begins `renderPass` on `framebuffer` and sets the viewport (y up) and scissor to cover `extent`
        void BeginRenderPass(const VulkanRenderPass& renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent,
                             const std::array<float, 4>& clearColor);
        bool IsInRenderPass() const { return m_RenderPass != nullptr; }

        // ICommandList: recording (the buffer must be in the RECORDING state)
        virtual void BeginRenderPass(IRenderTarget& target) override;
        virtual void EndRenderPass() override;
        virtual void BindPipeline(const IPipeline& pipeline) override;
        virtual void BindVertexBuffer(const IVertexBuffer& buffer) override;
        virtual void BindIndexBuffer(const IIndexBuffer& buffer) override;
        virtual void BindBindingSet(IBindingSet& bindingSet) override;
        virtual void PushConstants(const void* data, uint32_t size, uint32_t offset) override;
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;

        const vk::raii::CommandBuffer& Get() const { return m_CommandBuffer; }
        vk::CommandBuffer GetHandle() const { return *m_CommandBuffer; }

    private:
        const VulkanDevice* m_Device = nullptr;
        VulkanLiveResource m_LiveResource;

        vk::raii::CommandBuffer m_CommandBuffer = nullptr;
        CommandBufferState m_State = CommandBufferState::NOT_ALLOCATED;
        const VulkanRenderPass* m_RenderPass = nullptr; // the render pass being recorded, if any

        // Last pipeline bound while recording: descriptor sets and push constants use its layout
        const VulkanPipeline* m_BoundPipeline = nullptr;
        vk::PipelineLayout m_BoundPipelineLayout = nullptr;
    };
}
