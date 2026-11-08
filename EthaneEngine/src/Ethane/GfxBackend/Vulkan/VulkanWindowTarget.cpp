//
//  VulkanWindowTarget.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/2.
//

#include "ethpch.h"
#include "VulkanWindowTarget.h"
#include "VulkanUtils.h"

namespace Ethane {

VulkanWindowTarget::VulkanWindowTarget(VulkanContext* context, GLFWwindow* window)
    : m_Context(context), m_Window(window)
{
    const vk::raii::Instance& instance = m_Context->GetInstance();

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
    
    CreateSwapchain();
}

void VulkanWindowTarget::CreateSwapchain() {
    const vk::raii::Device& device = m_Context->GetDevice().Get();
    const auto& physicalDevice = m_Context->GetPhysicalDevice();

    const SwapChainSupportDetails swapChainSupport = physicalDevice.GetSwapchainSupport(*m_Surface);
    const vk::SurfaceCapabilitiesKHR& capabilities = swapChainSupport.capabilities;

    const vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapChainSupport.formats);
    const vk::PresentModeKHR presentMode = ChoosePresentMode(swapChainSupport.presentModes);
    m_Extent = ChooseExtent(capabilities);
    m_Width = m_Extent.width;
    m_Height = m_Extent.height;
    ETH_CORE_INFO("width: {0}, height: {1}", m_Width, m_Height);

    m_ImageFormat = surfaceFormat.format;
    m_ImageColorSpace = surfaceFormat.colorSpace;
    ETH_CORE_INFO("Swapchain image Format: {0} | Image Color Space: {1}", vk::to_string(m_ImageFormat), vk::to_string(m_ImageColorSpace));
    ETH_CORE_INFO("Supported Surface Usages: {0}", vk::to_string(capabilities.supportedUsageFlags));

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    m_MaxFramesInFlight = imageCount - 1;
    ETH_CORE_TRACE("Max frames in flight is {0}", m_MaxFramesInFlight);

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
        ETH_CORE_WARN("Clean up old swapchain");
        device.waitIdle();
        m_ImageViews.clear(); // views of the old images go first...
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
    
//    if (m_ImageAvailableSemaphores.empty() || m_RenderFinishedSemaphores.empty() || m_InFlightFences.empty())
//    {
//        ETH_CORE_TRACE("Create Synchronization Objects");
//        m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
//        m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
//        m_InFlightFences.resize(m_MaxFramesInFlight);
//        m_ImagesInFlight.resize(m_Images.size(), VK_NULL_HANDLE);
//
//        VkSemaphoreCreateInfo semaphoreInfo{};
//        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
//
//        VkFenceCreateInfo fenceInfo{};
//        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
//
//        for (size_t i = 0; i < m_MaxFramesInFlight; i++) {
//            VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
//            VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
//            VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]));
//        }
//    }

    ETH_CORE_INFO("SwapChain created");
}

void VulkanWindowTarget::Destroy() {
    if (*m_Swapchain || !m_ImageViews.empty()) {
        m_Context->GetDevice().Get().waitIdle();
        m_ImageViews.clear();
        m_Images.clear();
        m_Swapchain = nullptr;
    }
    if (*m_Surface) {
        ETH_CORE_INFO("Destroying Vulkan surface...");
        m_Surface = nullptr;
    }
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
