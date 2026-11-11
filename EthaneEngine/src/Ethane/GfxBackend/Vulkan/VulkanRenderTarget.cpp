#include "ethpch.h"
#include "VulkanRenderTarget.h"
#include "VulkanImageUtils.h"
#include "VulkanCommandBuffer.h"

namespace Ethane {

    namespace {
        std::atomic<uint64_t> s_NextImageId{ 1 };
    }

    VulkanRenderTarget::VulkanRenderTarget(const VulkanDevice* device, const RenderTargetSpecification& spec)
        : m_Device(device), m_Spec(spec)
    {
        ETH_CORE_ASSERT(spec.ColorFormat != ImageFormat::None && spec.ColorFormat != ImageFormat::Depth, "A render target needs a color format");
        const VulkanPhysicalDevice& physicalDevice = device->GetPhysicalDevice();
        m_ColorFormat = ImageUtils::VulkanImageFormat(spec.ColorFormat, physicalDevice);
        if (spec.DepthFormat != ImageFormat::None) {
            m_DepthFormat = ImageUtils::VulkanImageFormat(spec.DepthFormat, physicalDevice);
        }
        if (!physicalDevice.SupportsFormat(m_ColorFormat, vk::FormatFeatureFlagBits::eSampledImage)) {
            ETH_CORE_ERROR("Render target: {0} can't be sampled on this device", vk::to_string(m_ColorFormat));
        }

        // Format and depth checks happen here; ends in shader-read (color) / depth read-only layouts
        m_RenderPass = MakeRefCountPtr<VulkanRenderPass>(device, std::vector<ImageFormat>{ spec.ColorFormat }, spec.DepthFormat,
                                                         /*clearOnLoad*/ true, /*finalPresent*/ false);
        m_Spec.Width = std::max(m_Spec.Width, 1u);
        m_Spec.Height = std::max(m_Spec.Height, 1u);
        CreateImages();
    }

    void VulkanRenderTarget::Resize(uint32_t width, uint32_t height)
    {
        width = std::max(width, 1u);
        height = std::max(height, 1u);
        if (width == m_Spec.Width && height == m_Spec.Height) {
            return;
        }
        ETH_CORE_ASSERT(!m_Device->IsFrameInProgress(), "Resize a render target outside of frame recording");
        // Frames in flight may still draw into / sample the old images
        // TODO: deferred deletion instead of a full stall
        m_Device->Get().waitIdle();
        m_Spec.Width = width;
        m_Spec.Height = height;
        CreateImages();
    }

    VulkanRenderTarget::Attachment VulkanRenderTarget::CreateAttachment(vk::Format format, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect) const
    {
        const vk::raii::Device& device = m_Device->Get();
        Attachment attachment;
        attachment.Image = vk::raii::Image(device, vk::ImageCreateInfo{
            .imageType     = vk::ImageType::e2D,
            .format        = format,
            .extent        = { m_Spec.Width, m_Spec.Height, 1 },
            .mipLevels     = 1,
            .arrayLayers   = 1,
            .samples       = vk::SampleCountFlagBits::e1,
            .tiling        = vk::ImageTiling::eOptimal,
            .usage         = usage,
            .sharingMode   = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined}); // every render pass clears from undefined

        const vk::MemoryRequirements requirements = attachment.Image.getMemoryRequirements();
        attachment.Memory = vk::raii::DeviceMemory(device, vk::MemoryAllocateInfo{
            .allocationSize  = requirements.size,
            .memoryTypeIndex = m_Device->FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)});
        attachment.Image.bindMemory(*attachment.Memory, 0);

        attachment.View = vk::raii::ImageView(device, vk::ImageViewCreateInfo{
            .image            = *attachment.Image,
            .viewType         = vk::ImageViewType::e2D,
            .format           = format,
            .components       = {},
            .subresourceRange = {
                .aspectMask     = aspect,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1}});
        return attachment;
    }

    void VulkanRenderTarget::CreateImages()
    {
        // replaced in reverse creation order
        m_Framebuffer = nullptr;
        m_Depth.Reset();
        m_Color.Reset();

        using Usage = vk::ImageUsageFlagBits;
        m_Color = CreateAttachment(m_ColorFormat, Usage::eColorAttachment | Usage::eSampled, vk::ImageAspectFlagBits::eColor);
        std::vector<vk::ImageView> views{ *m_Color.View };
        if (m_DepthFormat != vk::Format::eUndefined) {
            m_Depth = CreateAttachment(m_DepthFormat, Usage::eDepthStencilAttachment, ImageUtils::DepthAttachmentAspect(m_DepthFormat));
            views.push_back(*m_Depth.View);
        }

        m_Framebuffer = vk::raii::Framebuffer(m_Device->Get(), vk::FramebufferCreateInfo{
            .renderPass      = m_RenderPass->GetHandle(),
            .attachmentCount = static_cast<uint32_t>(views.size()),
            .pAttachments    = views.data(),
            .width           = m_Spec.Width,
            .height          = m_Spec.Height,
            .layers          = 1});
        m_ImageId = s_NextImageId++;

        // Into the layout render passes leave it in, so it can be sampled (undefined contents) even before
        // the first pass draws into it
        VulkanCommandBuffer cmd(m_Device);
        cmd.AllocateAndBeginSingleUse(QueueFamilyTypes::Graphics);
        cmd.Get().pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
            vk::ImageMemoryBarrier{
                .srcAccessMask       = {},
                .dstAccessMask       = vk::AccessFlagBits::eShaderRead,
                .oldLayout           = vk::ImageLayout::eUndefined,
                .newLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *m_Color.Image,
                .subresourceRange    = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }});
        cmd.EndSingleUse(QueueFamilyTypes::Graphics);
    }

}
