//
//  VulkanGfxBackendAPI.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//

#include "ethpch.h"
#include "VulkanGfxBackendAPI.h"
#include "VulkanContext.h"

namespace Ethane {

void VulkanGfxBackendAPI::Init() {
    ContextCreateInfo ctxInfo{};
    ctxInfo.SetDebug(true);
    m_Context.Init(ctxInfo);
}

void VulkanGfxBackendAPI::Shutdown() {
    m_Context.Destroy();
}

void VulkanGfxBackendAPI::CreateWindowTarget(GLFWwindow* window) {
    
//    return glfwCreateWindowSurface(m_Context.m_VulkanInstance, window, nullptr, &m_Surface);
}

void VulkanGfxBackendAPI::BeginFrame() {
    
}

void VulkanGfxBackendAPI::EndFrame() {
    
}

}
