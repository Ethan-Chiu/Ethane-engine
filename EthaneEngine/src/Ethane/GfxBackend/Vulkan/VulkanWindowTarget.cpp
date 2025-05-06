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
    auto result = glfwCreateWindowSurface(m_Context->GetInstance(), m_Window, nullptr, &m_Surface);
    if (result != VK_SUCCESS) {
        ETH_CORE_ERROR("Surface creation failed");
    }
    
    if (!m_Context->GetPhysicalDevice().IsInitialized()) {
        m_Context->InitDevice(m_Surface);
    }
    
    CreateSwapchain();
}

void VulkanWindowTarget::CreateSwapchain() {
    VkSwapchainKHR oldSwapchain = m_Swapchain;

    VkDevice device = m_Context->GetDevice().GetHandle();
    auto& physicalDevice = m_Context->GetPhysicalDevice();

    SwapChainSupportDetails swapChainSupport = physicalDevice.GetSwapchainSupport(m_Surface);

    VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChoosePresentMode(swapChainSupport.presentModes);
    m_Extent = ChooseExtent(swapChainSupport.capabilities);
    m_Width = m_Extent.width;
    m_Height = m_Extent.height;
    ETH_CORE_INFO("width: {0}, height: {1}", m_Width, m_Height);

    m_ImageFormat = surfaceFormat.format;
    m_ImageColorSpace = surfaceFormat.colorSpace;
    ETH_CORE_INFO("Swapchain imgae Format: {0} | Image Color Space: {1}", m_ImageFormat, m_ImageColorSpace);

    VulkanSwapchainUtils::PrintSupportedSurfaceUsages(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    m_MaxFramesInFlight = imageCount - 1;
    ETH_CORE_TRACE("Max frames in flight is {0}", m_MaxFramesInFlight);

    // find the transformation of the surface
    VkSurfaceTransformFlagsKHR preTransform;
    if (swapChainSupport.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        // prefer a non-rotated transform
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = swapChainSupport.capabilities.currentTransform;
    }

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

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.surface = m_Surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = m_ImageFormat;
    swapchainCreateInfo.imageColorSpace = m_ImageColorSpace;
    swapchainCreateInfo.imageExtent = m_Extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // TODO: make this configurable?
    const auto& indices = physicalDevice.GetQueueFamilyIndices();
    if (indices.Graphics.value() != indices.Present.value()) {
        uint32_t queueFamilyIndices[] = { indices.Graphics.value(), indices.Present.value() };
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    swapchainCreateInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = oldSwapchain;

    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &m_Swapchain));

    if (oldSwapchain != VK_NULL_HANDLE)
    {
        ETH_CORE_WARN("Clean up old swapchain");
        DestroySwapchain(oldSwapchain);
    }

    // Get the swap chain images
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, m_Swapchain, &m_ImageCount, nullptr));
    m_Images.resize(m_ImageCount);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, m_Swapchain, &m_ImageCount, m_Images.data()));

    // create imageview
    m_ImageViews.resize(m_ImageCount);
    for (uint32_t i = 0; i < m_ImageCount; i++)
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = nullptr;
        colorAttachmentView.image = m_Images[i];
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.format = m_ImageFormat;
        colorAttachmentView.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        colorAttachmentView.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        colorAttachmentView.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        colorAttachmentView.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.flags = 0;

        VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &m_ImageViews[i]));
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


void VulkanWindowTarget::DestroySwapchain(VkSwapchainKHR swapchain) {
    VkDevice device = m_Context->GetDevice().GetHandle();
    vkDeviceWaitIdle(device);

    for (auto imageView : m_ImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void VulkanWindowTarget::Destroy() {
    if (m_Swapchain != VK_NULL_HANDLE) {
        DestroySwapchain(m_Swapchain);
        m_Swapchain = VK_NULL_HANDLE;
    }
    if (m_Surface != VK_NULL_HANDLE) {
        ETH_CORE_INFO("Destroying Vulkan surface...");
        vkDestroySurfaceKHR(m_Context->GetInstance(), m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }
}

VulkanWindowTarget::~VulkanWindowTarget() {
    Destroy();
}


VkSurfaceFormatKHR VulkanWindowTarget::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}


VkPresentModeKHR VulkanWindowTarget::ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D VulkanWindowTarget::ChooseExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        return surfaceCapabilities.currentExtent;
    }
    else {
        int width, height;
        width = m_Width;
        height = m_Height;
        // glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        return actualExtent;
    }
}

}
