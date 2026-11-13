#include "ethpch.h"
#include "VulkanUtils.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanBindingSet.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderTarget.h"


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
        m_BoundPipeline = nullptr;
        m_BoundPipelineLayout = nullptr;
        m_RenderPass = nullptr;
    }

    void VulkanCommandBuffer::End() {
        ETH_CORE_ASSERT(!m_RenderPass, "End() inside a render pass: EndRenderPass() first");
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

        // Submit, and wait for this command buffer only: queue.waitIdle() would also wait for every frame in
        // flight on the queue, a full stall (e.g. on each viewport resize)
        const vk::raii::Fence done(m_Device->Get(), vk::FenceCreateInfo{});
        const vk::CommandBuffer handle = *m_CommandBuffer;
        queue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &handle }, *done);
        [[maybe_unused]] const vk::Result result = m_Device->Get().waitForFences(*done, vk::True, UINT64_MAX);
        ETH_CORE_ASSERT(result == vk::Result::eSuccess, "Waiting for a single-use command buffer failed");

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

    void VulkanCommandBuffer::EndSingleUseAsync(QueueFamilyTypes type)
    {
        End();
        vk::raii::Fence done(m_Device->Get(), vk::FenceCreateInfo{});
        const vk::CommandBuffer handle = *m_CommandBuffer;
        m_Device->GetQueue(type).submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &handle }, *done);

        // Retired with the frame being recorded: by the time that's collected the work is long done, but the
        // fence is checked before the command buffer is freed, to be sure
        const vk::Device device = *m_Device->Get();
        const vk::Fence fence = *done;
        m_Device->Retire(VulkanDeletionQueue::Call([device, fence] { (void)device.waitForFences(fence, vk::True, UINT64_MAX); }),
                         std::move(m_CommandBuffer), std::move(done));
        m_CommandBuffer = nullptr;
        m_State = CommandBufferState::NOT_ALLOCATED;
    }


    void VulkanCommandBuffer::BindPipeline(const IPipeline& pipeline)
    {
        ETH_CORE_ASSERT(m_State == CommandBufferState::RECORDING, "Command buffer isn't recording");
        const auto& vulkanPipeline = static_cast<const VulkanPipeline&>(pipeline);
        vulkanPipeline.Bind(m_CommandBuffer);
        m_BoundPipeline = &vulkanPipeline;
        m_BoundPipelineLayout = vulkanPipeline.GetPipelineLayout();
    }

    void VulkanCommandBuffer::PushConstants(const void* data, uint32_t size, uint32_t offset)
    {
        ETH_CORE_ASSERT(m_State == CommandBufferState::RECORDING, "Command buffer isn't recording");
        ETH_CORE_ASSERT(m_BoundPipeline, "Bind a pipeline before pushing constants");
        ETH_CORE_ASSERT(data && size > 0 && size % 4 == 0 && offset % 4 == 0, "Push constants: size and offset must be non-zero multiples of 4");

        // Every stage whose range overlaps the written bytes must be named (and only stages that declare them)
        const auto& ranges = m_BoundPipeline->GetPushConstantRanges();
        vk::ShaderStageFlags stages;
        for (const vk::PushConstantRange& range : ranges) {
            if (range.offset < offset + size && offset < range.offset + range.size) {
                stages |= range.stageFlags;
            }
        }
        // The written bytes must all be declared: grow the covered span through ranges that touch it
        uint32_t coveredEnd = offset;
        for (bool grew = true; grew;) {
            grew = false;
            for (const vk::PushConstantRange& range : ranges) {
                if (range.offset <= coveredEnd && range.offset + range.size > coveredEnd) {
                    coveredEnd = range.offset + range.size;
                    grew = true;
                }
            }
        }
        if (!stages || coveredEnd < offset + size) {
            ETH_CORE_ERROR("Push constants [{0}, {1}) aren't declared by the bound pipeline's shader", offset, offset + size);
            ETH_CORE_ASSERT(false, "Push constants outside the shader's push-constant block");
            return;
        }
        m_CommandBuffer.pushConstants<uint8_t>(m_BoundPipelineLayout, stages, offset,
            vk::ArrayProxy<const uint8_t>(size, static_cast<const uint8_t*>(data)));
    }

    void VulkanCommandBuffer::BindBindingSet(IBindingSet& bindingSet)
    {
        ETH_CORE_ASSERT(m_State == CommandBufferState::RECORDING, "Command buffer isn't recording");
        ETH_CORE_ASSERT(m_BoundPipelineLayout, "Bind a pipeline before its binding sets");
        auto& vulkanSet = static_cast<VulkanBindingSet&>(bindingSet);
        const vk::DescriptorSet set = vulkanSet.Prepare();
        m_CommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_BoundPipelineLayout, vulkanSet.GetSetIndex(), set, {});
    }

    void VulkanCommandBuffer::BindVertexBuffer(const IVertexBuffer& buffer)
    {
        ETH_CORE_ASSERT(m_State == CommandBufferState::RECORDING, "Command buffer isn't recording");
        m_CommandBuffer.bindVertexBuffers(0, static_cast<const VulkanVertexBuffer&>(buffer).GetHandle(), vk::DeviceSize{ 0 });
    }

    void VulkanCommandBuffer::BindIndexBuffer(const IIndexBuffer& buffer)
    {
        ETH_CORE_ASSERT(m_State == CommandBufferState::RECORDING, "Command buffer isn't recording");
        m_CommandBuffer.bindIndexBuffer(static_cast<const VulkanIndexBuffer&>(buffer).GetHandle(), 0, vk::IndexType::eUint32);
    }

    void VulkanCommandBuffer::BeginRenderPass(const VulkanRenderPass& renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent,
                                              const std::array<float, 4>& clearColor)
    {
        ETH_CORE_ASSERT(m_State == CommandBufferState::RECORDING, "Command buffer isn't recording");
        ETH_CORE_ASSERT(!m_RenderPass, "BeginRenderPass() inside a render pass");
        renderPass.Begin(m_CommandBuffer, extent.width, extent.height, framebuffer, clearColor);
        m_RenderPass = &renderPass;

        // Dynamic state of every pipeline (see VulkanPipeline), valid for all pipelines bound in this pass.
        // The negative height flips Y so +Y is up in clip space (like OpenGL), which keeps counter-clockwise
        // front faces and GL-style projection matrices working; row 0 of the image is still its top.
        const float width = static_cast<float>(extent.width);
        const float height = static_cast<float>(extent.height);
        m_CommandBuffer.setViewport(0, vk::Viewport{ .x = 0.0f, .y = height, .width = width, .height = -height, .minDepth = 0.0f, .maxDepth = 1.0f });
        m_CommandBuffer.setScissor(0, vk::Rect2D{ .offset = { 0, 0 }, .extent = extent });
        m_CommandBuffer.setLineWidth(1.0f);
    }

    void VulkanCommandBuffer::BeginRenderPass(IRenderTarget& target)
    {
        auto& vulkanTarget = static_cast<VulkanRenderTarget&>(target);
        BeginRenderPass(vulkanTarget.GetVulkanRenderPass(), vulkanTarget.GetFramebuffer(), vulkanTarget.GetExtent(),
                        vulkanTarget.GetSpecification().ClearColor);
    }

    void VulkanCommandBuffer::EndRenderPass()
    {
        ETH_CORE_ASSERT(m_RenderPass, "EndRenderPass() without BeginRenderPass()");
        m_RenderPass->End(m_CommandBuffer);
        m_RenderPass = nullptr;
    }

    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        ETH_CORE_ASSERT(m_State == CommandBufferState::RECORDING, "Command buffer isn't recording");
        ETH_CORE_ASSERT(m_RenderPass, "Draw() outside a render pass");
        m_CommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        ETH_CORE_ASSERT(m_State == CommandBufferState::RECORDING, "Command buffer isn't recording");
        ETH_CORE_ASSERT(m_RenderPass, "DrawIndexed() outside a render pass");
        m_CommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}
