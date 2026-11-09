//
//  GfxBackendAPI.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//
#pragma once
#include "Ethane/Core/Base.h"
#include "Ethane/GfxBackend/Resource.h"
#include "GfxObject.h"

namespace Ethane {

class GfxBackendAPI
{
public:
    enum class API
    {
        None = 0, Vulkan = 1
    };
    
public:
    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    virtual RefCountPtr<IWindowTarget> CreateWindowTarget(void* windowHandle) = 0;

    virtual RefCountPtr<IVertexBuffer> CreateVertexBuffer(uint32_t size) = 0;
    virtual RefCountPtr<IIndexBuffer> CreateIndexBuffer(uint32_t size) = 0;
    virtual RefCountPtr<IUniformBuffer> CreateUniformBuffer(uint32_t size) = 0;

    // `pixels`: Width * Height texels of spec.Format, tightly packed, top row first
    virtual RefCountPtr<ITexture2D> CreateTexture2D(const TextureSpecification& spec, const void* pixels) = 0;
    // Loads an image file as RGBA8 (null if the file can't be read)
    RefCountPtr<ITexture2D> LoadTexture2D(const std::string& filepath, TextureFilter filter = TextureFilter::Linear, TextureWrap wrap = TextureWrap::Repeat);

    // Resources for descriptor set `set` of `shader`
    virtual RefCountPtr<IBindingSet> CreateBindingSet(const RefCountPtr<IShader>& shader, uint32_t set) = 0;

    virtual RefCountPtr<IShader> CreateShader(const std::string& filepath) = 0;
    virtual RefCountPtr<IRenderPass> CreateRenderPass(const std::vector<ImageFormat>& colorAttachmentFormats, ImageFormat depthAttachmentFormat, bool clearOnLoad, bool finalPresent = false) = 0;
    virtual RefCountPtr<IPipeline> CreatePipeline(const PipelineSpecification& spec) = 0;

    // Blocks until the GPU has finished all submitted work (e.g. before releasing resources it may use)
    virtual void WaitIdle() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    
public:
    virtual ~GfxBackendAPI() = default;
    static Scope<GfxBackendAPI> Create();
    
    inline static API GetAPI() { return s_API; }
    inline static void SetAPI(API _API) { s_API = _API; }
private:
    static API s_API;
};

}
