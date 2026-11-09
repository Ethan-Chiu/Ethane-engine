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
        bool hasDepth = (depthAttachmentFormat != ImageFormat::None);

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
            vk::AttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back(vk::AttachmentDescription{
                .format         = format,
                .samples        = vk::SampleCountFlagBits::e1,
                .loadOp         = clearOnLoad ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
                .storeOp        = vk::AttachmentStoreOp::eStore, // TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = clearOnLoad ? Layout::eUndefined : Layout::eDepthStencilReadOnlyOptimal});
            // The combined depth-stencil layouts are valid for depth-only formats too. Separate depth / stencil
            // layouts (eDepthAttachmentOptimal, ...) would need the separateDepthStencilLayouts feature.
            attachmentDescription.finalLayout = Layout::eDepthStencilReadOnlyOptimal; // TODO: eDepthStencilAttachmentOptimal if not sampling
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
            // TODO: configure
            subpassDependencies.push_back(vk::SubpassDependency{
                .srcSubpass    = VK_SUBPASS_EXTERNAL,
                .dstSubpass    = 0,
                .srcStageMask  = Stage::eColorAttachmentOutput,
                .dstStageMask  = Stage::eColorAttachmentOutput,
                .srcAccessMask = {},
                .dstAccessMask = Access::eColorAttachmentWrite});
        }

        if (hasDepth)
        {
            subpassDependencies.push_back(vk::SubpassDependency{
                .srcSubpass      = VK_SUBPASS_EXTERNAL,
                .dstSubpass      = 0,
                .srcStageMask    = Stage::eFragmentShader,
                .dstStageMask    = Stage::eEarlyFragmentTests,
                .srcAccessMask   = Access::eShaderRead,
                .dstAccessMask   = Access::eDepthStencilAttachmentWrite,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion});

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

    void VulkanRenderPass::Begin(const vk::raii::CommandBuffer& cmdBuffer, uint32_t width, uint32_t height, vk::Framebuffer frameBuffer) const
    {
        // NOTE: vulkan-hpp unions keep their constructors (VULKAN_HPP_NO_STRUCT_CONSTRUCTORS only affects structs)
        const std::array<vk::ClearValue, 2> clearValues = {
            vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } } },
            vk::ClearValue{ vk::ClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 } },
        };

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
