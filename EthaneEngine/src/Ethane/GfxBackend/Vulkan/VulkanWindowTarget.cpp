//
//  VulkanWindowTarget.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/2.
//

#include "ethpch.h"
#include "VulkanWindowTarget.h"
#include "VulkanUtils.h"
#include "VulkanImageUtils.h"

namespace Ethane {

VulkanWindowTarget::VulkanWindowTarget(VulkanContext* context, GLFWwindow* window)
    : m_Context(context), m_Window(window)
{
    const vk::raii::Instance& instance = m_Context->GetInstance();
    if (!*instance) {
        ETH_CORE_ERROR("Can't create a window target without a Vulkan instance");
        return;
    }

    // GLFW creates the surface as a raw handle; hand ownership to raii
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(*instance, m_Window, nullptr, &rawSurface) != VK_SUCCESS) {
        ETH_CORE_ERROR("Surface creation failed");
        return;
    }
    m_Surface = vk::raii::SurfaceKHR(instance, rawSurface);
    
    if (!m_Context->GetPhysicalDevice().IsInitialized()) {
        m_Context->InitDevice(*m_Surface);
    }

    const VulkanDevice& device = m_Context->GetDevice();
    m_Frames.reserve(kMaxFramesInFlight);
    for (uint32_t i = 0; i < kMaxFramesInFlight; i++) {
        m_Frames.emplace_back(device);
    }

    if (CreateSwapchain()) {
        CreateSwapchainResources();
    } else {
        m_NeedsRecreate = true; // window has no area yet, try again on the next frame
    }
}

VulkanWindowTarget::FrameData::FrameData(const VulkanDevice& device)
    : CommandBuffer(&device)
{
    CommandBuffer.Allocate(device.GetGraphicsCommandPool(), true);
    ImageAvailable = vk::raii::Semaphore(device.Get(), vk::SemaphoreCreateInfo{});
    // created signaled, so the first wait on each frame slot returns immediately
    InFlight = vk::raii::Fence(device.Get(), vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
}

bool VulkanWindowTarget::CreateSwapchain() {
    const vk::raii::Device& device = m_Context->GetDevice().Get();
    const auto& physicalDevice = m_Context->GetPhysicalDevice();

    const SwapChainSupportDetails swapChainSupport = physicalDevice.GetSwapchainSupport(*m_Surface);
    const vk::SurfaceCapabilitiesKHR& capabilities = swapChainSupport.capabilities;

    const vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapChainSupport.formats);
    const vk::PresentModeKHR presentMode = ChoosePresentMode(swapChainSupport.presentModes);
    const vk::Extent2D extent = ChooseExtent(capabilities);
    if (extent.width == 0 || extent.height == 0) {
        // e.g. minimized: a swapchain can't have a 0 extent, keep the current one until the window has an area again
        return false;
    }
    m_Extent = extent;
    ETH_CORE_INFO("width: {0}, height: {1}", m_Extent.width, m_Extent.height);

    m_ImageFormat = surfaceFormat.format;
    m_ImageColorSpace = surfaceFormat.colorSpace;
    ETH_CORE_INFO("Swapchain image Format: {0} | Image Color Space: {1}", vk::to_string(m_ImageFormat), vk::to_string(m_ImageColorSpace));
    ETH_CORE_INFO("Supported Surface Usages: {0}", vk::to_string(capabilities.supportedUsageFlags));

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    // find the transformation of the surface: prefer a non-rotated transform
    const vk::SurfaceTransformFlagBitsKHR preTransform =
        (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
            ? vk::SurfaceTransformFlagBitsKHR::eIdentity
            : capabilities.currentTransform;

    // test
    // // Find a supported composite alpha format (not all devices support alpha opaque)
    // VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // // Simply select the first composite alpha format available
    // std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
    //     VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    //     VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
    //     VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
    //     VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    // };
    // for (auto& compositeAlphaFlag : compositeAlphaFlags) {
    //     if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
    //         compositeAlpha = compositeAlphaFlag;
    //         break;
    //     };
    // }

    // queue family sharing: must stay alive until the swapchain is created
    const auto& indices = physicalDevice.GetQueueFamilyIndices();
    const std::array<uint32_t, 2> queueFamilyIndices = { indices.Graphics.value(), indices.Present.value() };
    const bool concurrent = indices.Graphics.value() != indices.Present.value();

    using Usage = vk::ImageUsageFlagBits;
    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface               = *m_Surface,
        .minImageCount         = imageCount,
        .imageFormat           = m_ImageFormat,
        .imageColorSpace       = m_ImageColorSpace,
        .imageExtent           = m_Extent,
        .imageArrayLayers      = 1,
        .imageUsage            = Usage::eColorAttachment | Usage::eStorage | Usage::eSampled, // TODO: make this configurable?
        .imageSharingMode      = concurrent ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = concurrent ? static_cast<uint32_t>(queueFamilyIndices.size()) : 0u,
        .pQueueFamilyIndices   = concurrent ? queueFamilyIndices.data() : nullptr,
        .preTransform          = preTransform,
        .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode           = presentMode,
        .clipped               = vk::True,
        .oldSwapchain          = *m_Swapchain}; // null on first creation

    // The old swapchain must stay alive until the new one is created from it
    vk::raii::SwapchainKHR newSwapchain(device, swapchainCreateInfo);
    if (*m_Swapchain)
    {
        ETH_CORE_INFO("Recreating swapchain ({0}x{1})", m_Extent.width, m_Extent.height);
        device.waitIdle();
        // everything referencing the old images goes first...
        m_RenderFinished.clear();
        m_Framebuffers.clear();
        m_ImageViews.clear();
    }
    m_Swapchain = std::move(newSwapchain); // ...then move-assignment destroys the old swapchain

    // Get the swap chain images
    m_Images = m_Swapchain.getImages();
    m_ImageCount = static_cast<uint32_t>(m_Images.size());

    // create image views
    m_ImageViews.reserve(m_ImageCount);
    for (const vk::Image image : m_Images)
    {
        m_ImageViews.emplace_back(device, vk::ImageViewCreateInfo{
            .image            = image,
            .viewType         = vk::ImageViewType::e2D,
            .format           = m_ImageFormat,
            .components       = {}, // identity swizzle
            .subresourceRange = {
                .aspectMask     = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1}});
    }

    ETH_CORE_INFO("SwapChain created");
    return true;
}

void VulkanWindowTarget::CreateSwapchainResources() {
    const VulkanDevice& device = m_Context->GetDevice();

    // The render pass only depends on the image format, which normally survives a recreation
    if (!m_RenderPass || m_RenderPassFormat != m_ImageFormat) {
        const ImageFormat format = ImageUtils::VulkanImageFormatToImageFormat(static_cast<VkFormat>(m_ImageFormat));
        ETH_CORE_ASSERT(format != ImageFormat::None, "Swapchain format has no ImageFormat equivalent");
        m_Framebuffers.clear(); // built against the old render pass
        m_RenderPass = MakeRefCountPtr<VulkanRenderPass>(&device, std::vector<ImageFormat>{ format }, ImageFormat::None, true, true);
        m_RenderPassFormat = m_ImageFormat;
    }

    m_Framebuffers.clear();
    m_Framebuffers.reserve(m_ImageCount);
    for (const vk::raii::ImageView& view : m_ImageViews) {
        const vk::ImageView attachment = *view;
        m_Framebuffers.emplace_back(device.Get(), vk::FramebufferCreateInfo{
            .renderPass      = m_RenderPass->GetHandle(),
            .attachmentCount = 1,
            .pAttachments    = &attachment,
            .width           = m_Extent.width,
            .height          = m_Extent.height,
            .layers          = 1});
    }

    m_RenderFinished.clear();
    m_RenderFinished.reserve(m_ImageCount);
    for (uint32_t i = 0; i < m_ImageCount; i++) {
        m_RenderFinished.emplace_back(device.Get(), vk::SemaphoreCreateInfo{});
    }
}

bool VulkanWindowTarget::RecreateSwapchain() {
    if (!CreateSwapchain()) {
        return false; // still 0-sized, m_NeedsRecreate stays set
    }
    CreateSwapchainResources();
    m_NeedsRecreate = false;
    return true;
}

void VulkanWindowTarget::Resize(uint32_t width, uint32_t height) {
    // The new extent is read from the surface when the swapchain is rebuilt; this only marks it stale.
    // Recreating here would be wasteful: a drag-resize sends many events per frame.
    ETH_CORE_TRACE("Window target resized to {0}x{1}", width, height);
    m_NeedsRecreate = true;
}

bool VulkanWindowTarget::BeginFrame() {
    ETH_CORE_ASSERT(!m_FrameStarted, "BeginFrame() called twice without EndFrame()");
    if (!*m_Surface) {
        return false;
    }
    if (m_NeedsRecreate && !RecreateSwapchain()) {
        return false;
    }

    const vk::raii::Device& device = m_Context->GetDevice().Get();
    FrameData& frame = m_Frames[m_CurrentFrame];

    // Wait until the GPU is done with this frame slot (its command buffer and semaphore are free again)
    [[maybe_unused]] const vk::Result waitResult = device.waitForFences(*frame.InFlight, vk::True, UINT64_MAX);
    ETH_CORE_ASSERT(waitResult == vk::Result::eSuccess, "Waiting for the in-flight fence failed");

    try {
        const auto [result, imageIndex] = m_Swapchain.acquireNextImage(UINT64_MAX, *frame.ImageAvailable, nullptr);
        ETH_CORE_ASSERT(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR, "Image acquisition timed out");
        if (result == vk::Result::eSuboptimalKHR) {
            m_NeedsRecreate = true; // still presentable: render this frame, rebuild before the next one
        }
        m_ImageIndex = imageIndex;
    } catch (const vk::OutOfDateKHRError&) {
        // nothing was acquired and the semaphore isn't signaled; the fence wasn't reset, so the slot stays usable
        m_NeedsRecreate = true;
        RecreateSwapchain();
        return false;
    }

    // Only reset once we know we'll submit: resetting and then bailing out would deadlock the next wait
    device.resetFences(*frame.InFlight);

    // Begin implicitly resets the buffer (the pool was created with eResetCommandBuffer)
    frame.CommandBuffer.Begin(true, false, false);
    const vk::raii::CommandBuffer& cmd = frame.CommandBuffer.Get();
    m_RenderPass->Begin(cmd, m_Extent.width, m_Extent.height, *m_Framebuffers[m_ImageIndex]);

    // Dynamic state of every pipeline (see VulkanPipeline). Set once here, it stays valid for all pipelines
    // bound later in this command buffer. The negative height flips Y so +Y is up in clip space (like OpenGL),
    // which keeps counter-clockwise front faces and GL-style projection matrices working.
    const float width = static_cast<float>(m_Extent.width);
    const float height = static_cast<float>(m_Extent.height);
    cmd.setViewport(0, vk::Viewport{ .x = 0.0f, .y = height, .width = width, .height = -height, .minDepth = 0.0f, .maxDepth = 1.0f });
    cmd.setScissor(0, vk::Rect2D{ .offset = { 0, 0 }, .extent = m_Extent });
    cmd.setLineWidth(1.0f);

    m_FrameStarted = true;
    return true;
}

ICommandList& VulkanWindowTarget::GetCommandList() {
    ETH_CORE_ASSERT(m_FrameStarted, "GetCommandList() is only valid between BeginFrame() and EndFrame()");
    return m_Frames[m_CurrentFrame].CommandBuffer;
}

void VulkanWindowTarget::EndFrame() {
    ETH_CORE_ASSERT(m_FrameStarted, "EndFrame() called without a successful BeginFrame()");
    m_FrameStarted = false;

    const VulkanDevice& device = m_Context->GetDevice();
    FrameData& frame = m_Frames[m_CurrentFrame];

    m_RenderPass->End(frame.CommandBuffer.Get());
    frame.CommandBuffer.End();

    // The color attachment write is the first use of the acquired image, so only that stage waits for it
    const vk::Semaphore waitSemaphore = *frame.ImageAvailable;
    const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::CommandBuffer commandBuffer = frame.CommandBuffer.GetHandle();
    const vk::Semaphore renderFinished = *m_RenderFinished[m_ImageIndex];

    device.GetGraphicsQueue().submit(vk::SubmitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &waitSemaphore,
        .pWaitDstStageMask    = &waitStage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &renderFinished}, *frame.InFlight);
    frame.CommandBuffer.UpdateSubmitted();

    const vk::SwapchainKHR swapchain = *m_Swapchain;
    try {
        const vk::Result result = device.GetPresentQueue().presentKHR(vk::PresentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &renderFinished,
            .swapchainCount     = 1,
            .pSwapchains        = &swapchain,
            .pImageIndices      = &m_ImageIndex});
        if (result == vk::Result::eSuboptimalKHR) {
            m_NeedsRecreate = true;
        }
    } catch (const vk::OutOfDateKHRError&) {
        m_NeedsRecreate = true; // rebuilt at the start of the next frame
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
}

void VulkanWindowTarget::Destroy() {
    if (*m_Swapchain || !m_Frames.empty()) {
        // frame slots may still be executing / being presented
        m_Context->GetDevice().Get().waitIdle();
        m_Frames.clear();
        m_RenderFinished.clear();
        m_Framebuffers.clear();
        m_RenderPass.reset();
        m_ImageViews.clear();
        m_Images.clear();
        m_Swapchain = nullptr;
    }
    if (*m_Surface) {
        ETH_CORE_INFO("Destroying Vulkan surface...");
        m_Surface = nullptr;
    }
    m_LiveResource.reset();
}

VulkanWindowTarget::~VulkanWindowTarget() {
    Destroy();
}


vk::SurfaceFormatKHR VulkanWindowTarget::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}


vk::PresentModeKHR VulkanWindowTarget::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const
{
    for (const auto availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo; // always supported
}


vk::Extent2D VulkanWindowTarget::ChooseExtent(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) const
{
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        return surfaceCapabilities.currentExtent;
    }

    // the surface lets us choose: use the window's framebuffer size (in pixels)
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);

    return vk::Extent2D{
        .width  = std::clamp(static_cast<uint32_t>(width), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
        .height = std::clamp(static_cast<uint32_t>(height), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)};
}

}
