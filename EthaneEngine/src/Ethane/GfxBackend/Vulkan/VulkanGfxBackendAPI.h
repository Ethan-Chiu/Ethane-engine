//
//  VulkanGfxBackendAPI.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//
#pragma once

#include "Ethane/GfxBackend/GfxBackendAPI.h"
#include "VulkanContext.h"
#include "Ethane/GfxBackend/WindowTarget.h"

struct GLFWwindow;

namespace Ethane {
    class VulkanGfxBackendAPI : public GfxBackendAPI
    {
    public:
        // Create the Vulkan application and the instance
        virtual void Init() override;
        virtual void Shutdown() override;
        
        virtual RefCountPtr<IWindowTarget> CreateWindowTarget(void* windowHandle) override;
        
        virtual void BeginFrame() override;
        virtual void EndFrame() override;
    public:
        ~VulkanGfxBackendAPI() = default;
        
    private:
        VulkanContext m_Context;
    };
}
