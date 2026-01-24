//
//  VulkanWindowTarget.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/1.
//

#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Ethane {

class VulkanWindowTarget : public IWindowTarget
{
public:
    VulkanWindowTarget(VulkanContext* context, GLFWwindow* window);
    ~VulkanWindowTarget();
    
    // Maybe don't need this
    virtual void Destroy() override;
    
private:
    void CreateSwapchain();
    void DestroySwapchain(VkSwapchainKHR swapchain);
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
private:
    VulkanContext* m_Context;
    
    GLFWwindow* m_Window;
    VkSurfaceKHR m_Surface;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    
    uint32_t m_Width = 0, m_Height = 0;
    VkExtent2D m_Extent;
    VkFormat m_ImageFormat;
    VkColorSpaceKHR m_ImageColorSpace;
    
    uint32_t m_MaxFramesInFlight = 2;
    uint32_t m_ImageCount = 0;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
};

namespace VulkanSwapchainUtils{

inline void PrintSupportedSurfaceUsages(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
    ETH_CORE_INFO("Supported Surface Usages:");

    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        ETH_CORE_INFO("  VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        ETH_CORE_INFO("  VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT)
        ETH_CORE_INFO("  VK_IMAGE_USAGE_SAMPLED_BIT");
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT)
        ETH_CORE_INFO("  VK_IMAGE_USAGE_STORAGE_BIT");
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        ETH_CORE_INFO("  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        ETH_CORE_INFO("  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT");
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
        ETH_CORE_INFO("  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT");
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
        ETH_CORE_INFO("  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT");
}

}
}
