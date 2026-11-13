#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"
#include "VulkanRenderPass.h"

namespace Ethane {

    class VulkanRenderTarget : public IRenderTarget
    {
    public:
        VulkanRenderTarget(const VulkanDevice* device, const RenderTargetSpecification& spec);
        ~VulkanRenderTarget() override { RetireImages(); }

        virtual const RenderTargetSpecification& GetSpecification() const override { return m_Spec; }
        virtual void Resize(uint32_t width, uint32_t height) override;
        virtual RefCountPtr<IRenderPass> GetRenderPass() const override { return RefCountPtr<IRenderPass>(m_RenderPass); }

        const VulkanRenderPass& GetVulkanRenderPass() const { return *m_RenderPass; }
        vk::Framebuffer GetFramebuffer() const { return *m_Framebuffer; }
        vk::Extent2D GetExtent() const { return { m_Spec.Width, m_Spec.Height }; }
        // In eShaderReadOnlyOptimal outside of the target's render passes
        vk::ImageView GetColorView() const { return *m_Color.View; }
        // Unique across all targets, and changes whenever the images are recreated: lets caches of
        // descriptors pointing at the color view (e.g. VulkanImGuiLayer) tell when theirs went stale
        uint64_t GetImageId() const { return m_ImageId; }

    private:
        // (Re)creates the images, views and framebuffer at m_Spec's size, retiring the old ones
        void CreateImages();
        // Hands the framebuffer, views, images and memory to the device's deletion queue: frames in flight
        // may still draw into / sample them
        void RetireImages();

        struct Attachment
        {
            // NOTE: declaration order matters, destroyed in reverse: view -> image -> memory
            vk::raii::DeviceMemory Memory = nullptr;
            vk::raii::Image Image = nullptr;
            vk::raii::ImageView View = nullptr;

            void Reset() { View = nullptr; Image = nullptr; Memory = nullptr; }
        };
        Attachment CreateAttachment(vk::Format format, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect) const;

    private:
        const VulkanDevice* m_Device;
        VulkanLiveResource m_LiveResource;
        RenderTargetSpecification m_Spec;
        vk::Format m_ColorFormat = vk::Format::eUndefined;
        vk::Format m_DepthFormat = vk::Format::eUndefined; // eUndefined: no depth buffer

        // NOTE: declaration order matters, destroyed in reverse: framebuffer -> attachments -> render pass
        RefCountPtr<VulkanRenderPass> m_RenderPass;
        Attachment m_Color;
        Attachment m_Depth;
        vk::raii::Framebuffer m_Framebuffer = nullptr;
        uint64_t m_ImageId = 0;
    };

}
