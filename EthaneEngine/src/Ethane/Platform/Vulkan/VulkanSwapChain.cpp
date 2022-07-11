#include "ethpch.h"

#define GLM_FORECE_DEPT_ZERO_TO_ONE // TODO: move
#include <GLFW/glfw3.h>

#include "VulkanSwapChain.h"
#include "VulkanImage.h"

#include "Ethane/Core/timer.h"

// test
#include "Ethane/Asset/ShaderLibrary.h";
#include "VulkanImGuiLayer.h"

namespace Ethane {

    VulkanSwapChain::~VulkanSwapChain()
    {
    }

    void VulkanSwapChain::Create(VkSurfaceKHR surface, const Ref<VulkanDevice>& _device, uint32_t width, uint32_t height, bool vsync)
    {
        // for profiling
        Timer timer;

        m_Surface = surface;
        m_Device = _device;
        m_PhysicalDevice = _device->GetPhysicalDevice();
        m_VSync = vsync;
        m_Width = width;
        m_Height = height;

        VkSwapchainKHR oldSwapchain = m_SwapChain;

        VkDevice device = m_Device->GetVulkanDevice();
        VkPhysicalDevice physicalDevice = m_PhysicalDevice->GetVulkanPhysicalDevice();


        SwapChainSupportDetails swapChainSupport = m_PhysicalDevice->QuerySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        m_Extent = ChooseSwapExtent(swapChainSupport.capabilities);
        ETH_CORE_INFO("width: {0}, height: {1}", m_Extent.width, m_Extent.height);

        m_ImageFormat = surfaceFormat.format;
        m_ImageColorSpace = surfaceFormat.colorSpace;
        ETH_CORE_INFO("Imgae Format: {0} | Image Color Space: {1}", m_ImageFormat, m_ImageColorSpace);

        m_DepthFormat = FindSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );

        // profiling
        ETH_CORE_TRACE("choose time: {0}ms", timer.ElapsedMillis());
        timer.Reset();

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // Find the transformation of the surface
        VkSurfaceTransformFlagsKHR preTransform;
        if (swapChainSupport.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            // We prefer a non-rotated transform
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
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const auto& indices = m_PhysicalDevice->GetQueueFamilyIndices();
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

        VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &m_SwapChain));

        // test
        // destroy the old swap chain, if exist
        // This also cleans up all the presentable images
        if (oldSwapchain != VK_NULL_HANDLE)
        {
            ETH_CORE_WARN("Clean up old swapchain");
            CleanupSwapChain(oldSwapchain);
        }

        // Get the swap chain images
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, m_SwapChain, &m_ImageCount, nullptr));
        m_Images.resize(m_ImageCount);
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, m_SwapChain, &m_ImageCount, m_Images.data()));

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

        m_DepthAttachment = VulkanImage2D::Create(m_Width, m_Height, 1, 1, m_DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true);
#if depth
#endif
        // Render pass
        CreateRenderPass();

        // Framebuffers 
        m_Framebuffers.resize(m_ImageViews.size());

        std::array<VkImageView, 1> attachments = {
            m_ImageViews[0],
            // m_DepthImageView
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_Extent.width;
        framebufferInfo.height = m_Extent.height;
        framebufferInfo.layers = 1;

        for (size_t i = 0; i < m_ImageViews.size(); i++) {
            attachments[0] = m_ImageViews[i];

            VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffers[i]));
        }

        // Create command pool TODO: don't create command pool here?
        if (m_CommandPool == nullptr)
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = m_PhysicalDevice->GetQueueFamilyIndices().Graphics.value();
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional 
            vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool);
        }

        // Command buffers
        m_CommandBuffers.resize(m_Framebuffers.size());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, m_CommandBuffers.data()));

        // Synchronization Objects
        if (m_ImageAvailableSemaphores.empty() || m_RenderFinishedSemaphores.empty() || m_InFlightFences.empty())
        {
            ETH_CORE_TRACE("Create Synchronization Objects");
            m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
            m_ImagesInFlight.resize(m_Images.size(), VK_NULL_HANDLE);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
                VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
                VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]));
            }
        }

        // profiling
        ETH_CORE_TRACE("create time: {0}ms", timer.ElapsedMillis());
        timer.Reset();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ETH_CORE_INFO("SwapChain created");
    }

    // private func
    VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
    {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
    {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
    {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
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

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    VkFormat VulkanSwapChain::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice->GetVulkanPhysicalDevice(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        ETH_CORE_ASSERT("failed to find supported format!");
    }

    void VulkanSwapChain::CreateRenderPass() {

        std::array<VkAttachmentDescription, 1> attachments = {};
        // Color attachment
        VkAttachmentDescription colorAttachment{};
        attachments[0].format = m_ImageFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

#if depth
        // Depth attachment
        VkAttachmentDescription depthAttachment{};
        attachments[1].format = m_DepthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

#endif

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
#if depth
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
#endif

        // TODO: test
#if subpassTest
        VkSubpassDescription subpass2{};
        subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass2.colorAttachmentCount = 1;
        subpass2.pColorAttachments = &colorAttachmentRef;
        VkSubpassDescription subpasses[] = { subpass, subpass2 };
        VkSubpassDependency dependency2{};
        dependency2.srcSubpass = 0;
        dependency2.dstSubpass = 1;
        dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency2.srcAccessMask = 0;
        dependency2.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency2.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        //
#endif
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // TODO: test
        VkSubpassDependency dependencies[] = { dependency };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1; // TODO
        renderPassInfo.pSubpasses = &subpass; // TODO
        renderPassInfo.dependencyCount = 1; // TODO
        renderPassInfo.pDependencies = dependencies; // TODO

        VkDevice device = m_Device->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass));
    }

    void VulkanSwapChain::OnResize(uint32_t width, uint32_t height)
    {
        ETH_CORE_WARN("VulkanSwapChain::OnResize");
        // NOTE: width == 0 or height == 0 will not occur
        m_Width = width;
        m_Height = height;
        m_NeedResize = true;
    }

    bool VulkanSwapChain::Resize()
    {
        if (m_IsRecreating) {
            ETH_CORE_WARN("Already recreating swapchain");
            return false;
        }

        m_IsRecreating = true;

        auto device = m_Device->GetVulkanDevice();
        vkDeviceWaitIdle(device);
        
        Create(m_Surface, m_Device, m_Width, m_Height, m_VSync);

        m_NeedResize = false;
        m_IsRecreating = false;
        return true;
    }

    bool VulkanSwapChain::BeginFrame()
    {
        auto device = m_Device->GetVulkanDevice();

        if(m_IsRecreating) {
            VK_CHECK_RESULT(vkDeviceWaitIdle(device));
            return false;
        }

        if (m_NeedResize) {
            VK_CHECK_RESULT(vkDeviceWaitIdle(device));
            Resize();
            return false;
        }

        // TODO: check result
        vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

        if (!AcquireNextImage()) {
            return false;
        }

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional
        VkCommandBuffer currentCommandBuffer = m_CommandBuffers[m_CurrentFrame];
        VK_CHECK_RESULT(vkResetCommandBuffer(currentCommandBuffer, 0));
        VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &beginInfo));

        std::array<VkClearValue, 1> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
#if depth
        clearValues[1].depthStencil = { 1.0f, 0 };
#endif        
        VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_Framebuffers[m_CurrentImageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_Extent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        renderPassInfo.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(currentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        // VkViewport viewport{};
        // viewport.x = 0.0f;
        // viewport.y = 0.0f;
        // viewport.width = (float)m_Extent.width;
        // viewport.height = (float)m_Extent.height;
        // viewport.minDepth = 0.0f;
        // viewport.maxDepth = 1.0f;
        // vkCmdSetViewport(currentCommandBuffer, 0, 1, &viewport);
        // 
        // VkRect2D scissor{};
        // scissor.offset = { 0, 0 };
        // scissor.extent = m_Extent;
        // vkCmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

        return true;
    }

    void VulkanSwapChain::EndFrame()
    {
        auto device = m_Device->GetVulkanDevice();
        VkCommandBuffer currentCommandBuffer = m_CommandBuffers[m_CurrentFrame];

        // vkCmdNextSubpass(currentCommandBuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        std::vector<VkCommandBuffer> secondaryCommandBuffers;
        secondaryCommandBuffers.push_back((VulkanImGuiLayer::GetImGuiCommandBuffer())[m_CurrentFrame]);

        vkCmdExecuteCommands(currentCommandBuffer, uint32_t(secondaryCommandBuffers.size()), secondaryCommandBuffers.data());
        
        vkCmdEndRenderPass(m_CommandBuffers[m_CurrentFrame]);
        VK_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffers[m_CurrentFrame]));
       

        if (m_ImagesInFlight[m_CurrentImageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(device, 1, &m_ImagesInFlight[m_CurrentImageIndex], VK_TRUE, UINT64_MAX); // wait for signaled // TODO: check result
        }
        m_ImagesInFlight[m_CurrentImageIndex] = m_InFlightFences[m_CurrentFrame];

        VK_CHECK_RESULT(vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrame]));


        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        
        VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.pWaitDstStageMask = waitStages;
        
        VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];

        VK_CHECK_RESULT(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]));

        // Update command buffer state

        Present(m_Device->GetGraphicsQueue(), m_RenderFinishedSemaphores[m_CurrentFrame]);

        m_CurrentFrame = (++m_CurrentFrame) % MAX_FRAMES_IN_FLIGHT;
    }


    bool VulkanSwapChain::AcquireNextImage()
    {
        auto device = m_Device->GetVulkanDevice(); // TODO: save device as member

        VkResult result = vkAcquireNextImageKHR(device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_CurrentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            Resize();
            return false;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            ETH_CORE_ERROR("Swapchain require next image failed");
            return false;
        }
        return true;
    }

    void VulkanSwapChain::Present(VkQueue queue, VkSemaphore signalSemaphore)
    {
        ETH_PROFILE_FUNCTION();

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.pNext = nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_SwapChain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;
        // Check if a wait semaphore has been specified to wait for before presenting the image
        if (signalSemaphore != nullptr)
        {
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &signalSemaphore;
        }
        else 
        {
            ETH_CORE_WARN("Signal semaphores for queue present is not provided!");
        }

        VkResult result = vkQueuePresentKHR(queue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            Resize();
        }
        else if(result != VK_SUCCESS)
        {
            ETH_CORE_ASSERT("Queue present error");
        }
    }

    void VulkanSwapChain::CleanupSwapChain(VkSwapchainKHR swapchain) {
        VkDevice device = m_Device->GetVulkanDevice();

        vkDeviceWaitIdle(device);

        vkFreeCommandBuffers(device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

        for (auto framebuffer : m_Framebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyRenderPass(device, m_RenderPass, nullptr); // test

        m_DepthAttachment->Destroy();

        for (auto imageView : m_ImageViews) 
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }

    void VulkanSwapChain::Destroy() {
        VkDevice device = m_Device->GetVulkanDevice();

        vkDeviceWaitIdle(device);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, m_InFlightFences[i], nullptr);
        }

        CleanupSwapChain(m_SwapChain);

        // destroy commandpool
        vkDestroyCommandPool(device, m_CommandPool, nullptr);

        VulkanImGuiLayer::Cleanup();
    }

}
