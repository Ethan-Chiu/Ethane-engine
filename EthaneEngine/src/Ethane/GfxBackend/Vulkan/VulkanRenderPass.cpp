#include "ethpch.h"
#include "VulkanUtils.h"
#include "VulkanRenderPass.h"
#include "VulkanContext.h"
#include "VulkanImageUtils.h"

namespace Ethane {

    VulkanRenderPass::VulkanRenderPass(const VulkanDevice* device, const std::vector<ImageFormat>& colorAttachmentFormats, ImageFormat depthAttachmentFormat, bool clearOnLoad, bool finalPresent)
        :m_Device(device)
    {
        using Layout = vk::ImageLayout;

        std::vector<vk::AttachmentDescription> attachmentDescriptions;
        std::vector<vk::AttachmentReference> colorAttachmentReferences;
        vk::AttachmentReference depthAttachmentReference;

        using FormatFeature = vk::FormatFeatureFlagBits;
        const VulkanPhysicalDevice& physicalDevice = m_Device->GetPhysicalDevice();

        uint32_t attachmentIndex = 0;
        const bool hasDepth = (depthAttachmentFormat != ImageFormat::None);
        m_HasDepth = hasDepth;

        m_AttachmentCount = colorAttachmentFormats.size();
        for (auto attachmentFormat : colorAttachmentFormats)
        {
            const vk::Format format = ImageUtils::VulkanImageFormat(attachmentFormat, physicalDevice);
            if (!physicalDevice.SupportsFormat(format, FormatFeature::eColorAttachment)) {
                ETH_CORE_ERROR("Render pass: {0} can't be used as a color attachment on this device", vk::to_string(format));
                ETH_CORE_ASSERT(false, "Unsupported color attachment format");
            }
            attachmentDescriptions.push_back(vk::AttachmentDescription{
                .format         = format,
                .samples        = vk::SampleCountFlagBits::e1,
                .loadOp         = clearOnLoad ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
                .storeOp        = vk::AttachmentStoreOp::eStore, // TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = clearOnLoad ? Layout::eUndefined : Layout::eShaderReadOnlyOptimal,
                .finalLayout    = finalPresent ? Layout::ePresentSrcKHR : Layout::eShaderReadOnlyOptimal});

            colorAttachmentReferences.push_back(vk::AttachmentReference{ .attachment = attachmentIndex, .layout = Layout::eColorAttachmentOptimal });

            attachmentIndex++;
        }

        if (hasDepth)
        {
            // ImageFormat::Depth resolves to the device's preferred format, explicit formats are checked
            const vk::Format format = ImageUtils::VulkanImageFormat(depthAttachmentFormat, physicalDevice);
            ETH_CORE_ASSERT(ImageUtils::IsDepthFormat(format), "Depth attachment needs a depth format");
            if (!physicalDevice.SupportsFormat(format, FormatFeature::eDepthStencilAttachment)) {
                ETH_CORE_ERROR("Render pass: {0} can't be used as a depth attachment on this device (use ImageFormat::Depth for the device default: {1})",
                               vk::to_string(format), vk::to_string(physicalDevice.GetDepthFormat()));
                ETH_CORE_ASSERT(false, "Unsupported depth attachment format");
            }
            // A presenting pass (the window) never reads its depth afterwards: don't store it and leave it in
            // the attachment layout. Offscreen passes keep it for sampling.
            // The combined depth-stencil layouts are valid for depth-only formats too. Separate depth / stencil
            // layouts (eDepthAttachmentOptimal, ...) would need the separateDepthStencilLayouts feature.
            const Layout keptLayout = finalPresent ? Layout::eDepthStencilAttachmentOptimal : Layout::eDepthStencilReadOnlyOptimal;
            attachmentDescriptions.push_back(vk::AttachmentDescription{
                .format         = format,
                .samples        = vk::SampleCountFlagBits::e1,
                .loadOp         = clearOnLoad ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
                .storeOp        = finalPresent ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore,
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = clearOnLoad ? Layout::eUndefined : keptLayout,
                .finalLayout    = keptLayout});
            depthAttachmentReference = vk::AttachmentReference{ .attachment = attachmentIndex, .layout = Layout::eDepthStencilAttachmentOptimal };
            attachmentIndex++;
        }

        std::vector<vk::SubpassDescription> subpasses;
        std::vector<vk::SubpassDependency>  subpassDependencies;

        subpasses.push_back(vk::SubpassDescription{
            .pipelineBindPoint       = vk::PipelineBindPoint::eGraphics,
            .colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentReferences.size()),
            .pColorAttachments       = colorAttachmentReferences.data(),
            .pDepthStencilAttachment = hasDepth ? &depthAttachmentReference : nullptr});

        using Stage = vk::PipelineStageFlagBits;
        using Access = vk::AccessFlagBits;

        if (colorAttachmentFormats.size())
        {
            // Before the clear / first write. Presenting: the window waits on the acquire semaphore at the
            // color output stage, which this chains to. Offscreen: shaders of the previous frame may still be
            // sampling the image (write after read: an execution dependency is enough).
            subpassDependencies.push_back(vk::SubpassDependency{
                .srcSubpass    = VK_SUBPASS_EXTERNAL,
                .dstSubpass    = 0,
                .srcStageMask  = finalPresent ? vk::PipelineStageFlags(Stage::eColorAttachmentOutput)
                                              : Stage::eColorAttachmentOutput | Stage::eFragmentShader,
                .dstStageMask  = Stage::eColorAttachmentOutput,
                .srcAccessMask = {},
                .dstAccessMask = Access::eColorAttachmentWrite});
        }

        if (colorAttachmentFormats.size() && !finalPresent)
        {
            // Colors written here are sampled by later passes (e.g. ImGui showing the image)
            subpassDependencies.push_back(vk::SubpassDependency{
                .srcSubpass    = 0,
                .dstSubpass    = VK_SUBPASS_EXTERNAL,
                .srcStageMask  = Stage::eColorAttachmentOutput,
                .dstStageMask  = Stage::eFragmentShader,
                .srcAccessMask = Access::eColorAttachmentWrite,
                .dstAccessMask = Access::eShaderRead});
        }

        if (hasDepth)
        {
            // Before the clear / first depth test: earlier users of the same depth image must be done. That's
            // the previous frame's depth tests (one depth image is shared by all frames in flight: write after
            // write) or, for an offscreen pass, shaders that sampled it (write after read).
            subpassDependencies.push_back(vk::SubpassDependency{
                .srcSubpass    = VK_SUBPASS_EXTERNAL,
                .dstSubpass    = 0,
                .srcStageMask  = Stage::eEarlyFragmentTests | Stage::eLateFragmentTests | Stage::eFragmentShader,
                .dstStageMask  = Stage::eEarlyFragmentTests | Stage::eLateFragmentTests,
                .srcAccessMask = Access::eDepthStencilAttachmentWrite,
                .dstAccessMask = Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite});
        }

        if (hasDepth && !finalPresent)
        {
            // Depth written here is sampled by later passes
            subpassDependencies.push_back(vk::SubpassDependency{
                .srcSubpass      = 0,
                .dstSubpass      = VK_SUBPASS_EXTERNAL,
                .srcStageMask    = Stage::eLateFragmentTests,
                .dstStageMask    = Stage::eFragmentShader,
                .srcAccessMask   = Access::eDepthStencilAttachmentWrite,
                .dstAccessMask   = Access::eShaderRead,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion});
        }

        // Create the actual renderpass
        m_RenderPass = vk::raii::RenderPass(m_Device->Get(), vk::RenderPassCreateInfo{
            .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
            .pAttachments    = attachmentDescriptions.data(),
            .subpassCount    = static_cast<uint32_t>(subpasses.size()),
            .pSubpasses      = subpasses.data(),
            .dependencyCount = static_cast<uint32_t>(subpassDependencies.size()),
            .pDependencies   = subpassDependencies.data()});
    }

    void VulkanRenderPass::Begin(const vk::raii::CommandBuffer& cmdBuffer, uint32_t width, uint32_t height, vk::Framebuffer frameBuffer,
                                 const std::array<float, 4>& clearColor) const
    {
        // One clear value per attachment, indexed like the attachments: colors first, then depth
        // NOTE: vulkan-hpp unions keep their constructors (VULKAN_HPP_NO_STRUCT_CONSTRUCTORS only affects structs)
        std::vector<vk::ClearValue> clearValues(m_AttachmentCount,
            vk::ClearValue{ vk::ClearColorValue{ clearColor } });
        if (m_HasDepth) {
            clearValues.push_back(vk::ClearValue{ vk::ClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 } });
        }

        cmdBuffer.beginRenderPass(vk::RenderPassBeginInfo{
            .renderPass      = *m_RenderPass,
            .framebuffer     = frameBuffer,
            .renderArea      = vk::Rect2D{ .offset = { 0, 0 }, .extent = { width, height } },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues    = clearValues.data()}, vk::SubpassContents::eInline);
        // cmdBuffer state change
    }

    void VulkanRenderPass::End(const vk::raii::CommandBuffer& cmdBuffer) const
    {
        cmdBuffer.endRenderPass();
        // cmdBuffer state change
    }

}
