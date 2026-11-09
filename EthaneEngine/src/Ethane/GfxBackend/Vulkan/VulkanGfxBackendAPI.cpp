//
//  VulkanGfxBackendAPI.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//

#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/GfxBackend/Vulkan/VulkanPipeline.h"
#include "Ethane/GfxBackend/Vulkan/VulkanRenderPass.h"
#include "ethpch.h"
#include "Ethane/GfxBackend/Resource.h"
#include "VulkanGfxBackendAPI.h"
#include "VulkanContext.h"
#include "VulkanWindowTarget.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanBindingSet.h"


namespace Ethane {

void VulkanGfxBackendAPI::Init() {
    ContextCreateInfo ctxInfo{};
    ctxInfo.SetDebug(true);
    m_Context.Init(ctxInfo);
}

void VulkanGfxBackendAPI::Shutdown() {
    ReleaseDefaultResources(); // before the device goes
    m_Context.Destroy();
}

RefCountPtr<IWindowTarget> VulkanGfxBackendAPI::CreateWindowTarget(void* window) {
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window);
    return MakeRefCountPtr<VulkanWindowTarget>(&m_Context, glfwWindow);
}

RefCountPtr<IVertexBuffer> VulkanGfxBackendAPI::CreateVertexBuffer(uint32_t size) {
    return MakeRefCountPtr<VulkanVertexBuffer>(&m_Context.GetDevice(), size);
}

RefCountPtr<IIndexBuffer> VulkanGfxBackendAPI::CreateIndexBuffer(uint32_t size) {
    return MakeRefCountPtr<VulkanIndexBuffer>(&m_Context.GetDevice(), size);
}

RefCountPtr<IUniformBuffer> VulkanGfxBackendAPI::CreateUniformBuffer(uint32_t size) {
    return MakeRefCountPtr<VulkanUniformBuffer>(&m_Context.GetDevice(), size);
}

RefCountPtr<ITexture2D> VulkanGfxBackendAPI::CreateTexture2D(const TextureSpecification& spec, const void* pixels) {
    return MakeRefCountPtr<VulkanTexture2D>(&m_Context.GetDevice(), spec, pixels);
}

RefCountPtr<IBindingSet> VulkanGfxBackendAPI::CreateBindingSet(const RefCountPtr<IShader>& shader, uint32_t set) {
    return MakeRefCountPtr<VulkanBindingSet>(&m_Context.GetDevice(), dynamic_pointer_cast<VulkanShader>(shader), set);
}

void VulkanGfxBackendAPI::WaitIdle() {
    if (m_Context.GetDevice().IsInitialized()) {
        m_Context.GetDevice().Get().waitIdle();
    }
}

RefCountPtr<IShader> VulkanGfxBackendAPI::CreateShader(const std::string& filepath) {
    return MakeRefCountPtr<VulkanShader>(&m_Context.GetDevice(), filepath);
}

RefCountPtr<IRenderPass> VulkanGfxBackendAPI::CreateRenderPass(const std::vector<ImageFormat>& colorAttachmentFormats, ImageFormat depthAttachmentFormat, bool clearOnLoad, bool finalPresent) {
    return MakeRefCountPtr<VulkanRenderPass>(&m_Context.GetDevice(), colorAttachmentFormats, depthAttachmentFormat, clearOnLoad, finalPresent);
}

RefCountPtr<IPipeline> VulkanGfxBackendAPI::CreatePipeline(const PipelineSpecification& spec) {
    return MakeRefCountPtr<VulkanPipeline>(&m_Context.GetDevice(), spec);
}

void VulkanGfxBackendAPI::BeginFrame() {
    
}

void VulkanGfxBackendAPI::EndFrame() {
    
}

}
