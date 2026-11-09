//
//  VulkanGfxBackendAPI.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//
#pragma once

#include "Ethane/GfxBackend/GfxBackendAPI.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/GfxBackend/Resource.h"
#include "VulkanContext.h"

struct GLFWwindow;

namespace Ethane {
    class VulkanGfxBackendAPI : public GfxBackendAPI
    {
    public:
        // Create the Vulkan application and the instance
        virtual void Init() override;
        virtual void Shutdown() override;
        
        virtual RefCountPtr<IWindowTarget> CreateWindowTarget(void* windowHandle) override;

        virtual RefCountPtr<IVertexBuffer> CreateVertexBuffer(uint32_t size) override;
        virtual RefCountPtr<IIndexBuffer> CreateIndexBuffer(uint32_t size) override;
        virtual RefCountPtr<IUniformBuffer> CreateUniformBuffer(uint32_t size) override;
        virtual RefCountPtr<ITexture2D> CreateTexture2D(const TextureSpecification& spec, const void* pixels) override;
        virtual RefCountPtr<IBindingSet> CreateBindingSet(const RefCountPtr<IShader>& shader, uint32_t set) override;

        virtual RefCountPtr<IShader> CreateShader(const std::string& filepath) override;
        virtual RefCountPtr<IRenderPass> CreateRenderPass(const std::vector<ImageFormat>& colorAttachmentFormats, ImageFormat depthAttachmentFormat, bool clearOnLoad, bool finalPresent = false) override;
        virtual RefCountPtr<IPipeline> CreatePipeline(const PipelineSpecification& spec) override;

        virtual void WaitIdle() override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;
    public:
        ~VulkanGfxBackendAPI() = default;
        
    private:
        VulkanContext m_Context;
    };
}
