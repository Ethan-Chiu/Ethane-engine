//
//  VulkanWindowTarget.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/1.
//

#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRenderPass.h"
#include <GLFW/glfw3.h>

namespace Ethane {

class VulkanWindowTarget : public IWindowTarget
{
public:
    // Frames the CPU may record ahead of the GPU. Each frame slot has its own command buffer,
    // acquire semaphore and fence, so slot N can be recorded while slot N-1 is still executing.
    static constexpr uint32_t kMaxFramesInFlight = 2;

    // NOTE: `context` must outlive this window target (the swapchain belongs to its device)
    VulkanWindowTarget(VulkanContext* context, GLFWwindow* window);
    ~VulkanWindowTarget();
    
    // Releases frame data -> framebuffers -> render pass -> image views -> swapchain -> surface. Safe to call more than once.
    virtual void Destroy() override;

    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual bool BeginFrame() override;
    virtual void EndFrame() override;

    // Valid between BeginFrame() and EndFrame(): the command buffer recording the frame,
    // already inside the window's render pass (which clears the image)
    const VulkanCommandBuffer& GetCurrentCommandBuffer() const { return m_Frames[m_CurrentFrame].CommandBuffer; }
    const VulkanRenderPass& GetRenderPass() const { return *m_RenderPass; }
    vk::Extent2D GetExtent() const { return m_Extent; }
    
private:
    struct FrameData
    {
        explicit FrameData(const VulkanDevice& device);

        // NOTE: declaration order matters, members are destroyed in reverse: fence -> semaphore -> command buffer
        VulkanCommandBuffer CommandBuffer;
        vk::raii::Semaphore ImageAvailable = nullptr; // signaled by acquire, waited on by the submit
        vk::raii::Fence InFlight = nullptr;           // signaled when the GPU has finished this frame slot
    };

    // Creates the swapchain, or recreates it from the current one (e.g. after a resize).
    // Returns false (and leaves the current swapchain alone) when the surface has a 0 extent.
    bool CreateSwapchain();
    // Image-dependent objects: render pass (if the format changed), framebuffers, render-finished semaphores
    void CreateSwapchainResources();
    // Waits for the GPU, then rebuilds the swapchain and everything that depends on its images
    bool RecreateSwapchain();

    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    
private:
    VulkanContext* m_Context;
    GLFWwindow* m_Window;
    // Released in Destroy(): the object may outlive its Vulkan objects (Application destroys it explicitly)
    std::optional<VulkanLiveResource> m_LiveResource{ std::in_place };

    // NOTE: declaration order matters, members are destroyed in reverse:
    // frames -> render-finished semaphores -> framebuffers -> render pass -> image views -> swapchain -> surface
    vk::raii::SurfaceKHR m_Surface = nullptr;
    vk::raii::SwapchainKHR m_Swapchain = nullptr;
    std::vector<vk::Image> m_Images;               // owned by the swapchain, not destroyed by us
    std::vector<vk::raii::ImageView> m_ImageViews;
    RefCountPtr<VulkanRenderPass> m_RenderPass;    // clears the image, ends in PRESENT_SRC
    std::vector<vk::raii::Framebuffer> m_Framebuffers;
    // One per swapchain image, not per frame slot: the presentation engine may still be waiting on it
    // after the frame's fence signals, so it can only be reused once the same image is acquired again
    std::vector<vk::raii::Semaphore> m_RenderFinished;
    std::vector<FrameData> m_Frames;               // kMaxFramesInFlight entries
    
    vk::Extent2D m_Extent;
    vk::Format m_ImageFormat = vk::Format::eUndefined;
    vk::Format m_RenderPassFormat = vk::Format::eUndefined;
    vk::ColorSpaceKHR m_ImageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    
    uint32_t m_ImageCount = 0;
    uint32_t m_CurrentFrame = 0;   // frame slot, cycles through kMaxFramesInFlight
    uint32_t m_ImageIndex = 0;     // swapchain image acquired for the current frame
    bool m_FrameStarted = false;
    bool m_NeedsRecreate = false;  // set by Resize() or an out-of-date/suboptimal acquire/present
};

}
