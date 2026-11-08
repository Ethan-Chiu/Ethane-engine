//
//  VulkanWindowTarget.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/1.
//

#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"
#include <GLFW/glfw3.h>

namespace Ethane {

class VulkanWindowTarget : public IWindowTarget
{
public:
    // NOTE: `context` must outlive this window target (the swapchain belongs to its device)
    VulkanWindowTarget(VulkanContext* context, GLFWwindow* window);
    ~VulkanWindowTarget();
    
    // Releases image views -> swapchain -> surface. Safe to call more than once.
    virtual void Destroy() override;
    
private:
    // Creates the swapchain, or recreates it from the current one (e.g. after a resize)
    void CreateSwapchain();
    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    
private:
    VulkanContext* m_Context;
    GLFWwindow* m_Window;
    // Released in Destroy(): the object may outlive its Vulkan objects (Application destroys it explicitly)
    std::optional<VulkanLiveResource> m_LiveResource{ std::in_place };

    // NOTE: declaration order matters, members are destroyed in reverse: image views -> swapchain -> surface
    vk::raii::SurfaceKHR m_Surface = nullptr;
    vk::raii::SwapchainKHR m_Swapchain = nullptr;
    std::vector<vk::Image> m_Images;               // owned by the swapchain, not destroyed by us
    std::vector<vk::raii::ImageView> m_ImageViews;
    
    uint32_t m_Width = 0, m_Height = 0;
    vk::Extent2D m_Extent;
    vk::Format m_ImageFormat = vk::Format::eUndefined;
    vk::ColorSpaceKHR m_ImageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    
    uint32_t m_MaxFramesInFlight = 2;
    uint32_t m_ImageCount = 0;
};

}
