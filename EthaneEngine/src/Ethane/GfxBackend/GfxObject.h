//
//  GfxObject.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2026/1/24.
//

#pragma once
#include "Resource.h"
#include "ShaderConfig.h"

namespace Ethane {

class IBuffer : public RefCounted<IBuffer>
{
public:
    virtual ~IBuffer() = default;
    virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
};

class IVertexBuffer : public IBuffer {};

class IIndexBuffer : public IBuffer {
public:
    virtual uint32_t GetCount() const = 0;
};

class IShader : public RefCounted<IShader>
{
public:
    virtual ~IShader() = default;
    virtual const std::string& GetName() const = 0;
};

enum class ImageFormat
{
    None = 0,
    RED32F,
    RGB,
    RGBA,
    RGBA16F,
    RGBA32F,
    RG16F,
    RG32F,
    SRGB,
    BGR,
    BGRA,

    DEPTH32F,
    DEPTH24STENCIL8,   // not supported on Apple GPUs
    DEPTH32FSTENCIL8,
    // Defaults
    Depth,             // the device's preferred depth-stencil attachment format, resolved by the backend
};

enum class ImageUsage
{
    None = 0,
    Texture,
    Attachment,
    Storage
};

enum class ImageLayout
{
    Undefined,
    General,
    ShaderRead,
    PresentSRC,
};

enum class AccessMask
{
    None,
    ShaderRead,
    ShaderWrite,
    ColorRead,
    ColorWrite,
    TransferRead,
    TransferWrite,
    MemoryRead,
    MemoryWrite,
};

enum class PipelineStage
{
    None,
    PipeTop,
    VertexShader,
    FragmentShader,
    ComputeShader,
    Transfer,
    PipeBottom,
};

class IRenderPass : public RefCounted<IRenderPass>
{
public:
    virtual ~IRenderPass() = default;
};

enum class CullMode {
    NONE,
    FRONT,
    BACK,
};

struct PipelineSpecification
{
    RefCountPtr<IShader> Shader;
    RefCountPtr<IRenderPass> RenderPass;
    VertexBufferLayout Layout;
    CullMode CullMode = CullMode::BACK;
};

struct ComputePipelineSpecification
{
    const IShader* Shader;
};

class IPipeline : public RefCounted<IPipeline>
{
public:
    virtual ~IPipeline() = default;

    virtual PipelineSpecification& GetSpecification() = 0;
    virtual const PipelineSpecification& GetSpecification() const = 0;
};

class IComputePipeline : public RefCounted<IComputePipeline>
{
public:
    virtual ~IComputePipeline() = default;

    virtual ComputePipelineSpecification& GetSpecification() = 0;
    virtual const ComputePipelineSpecification& GetSpecification() const = 0;

    virtual void Destroy() = 0;
};

// Records GPU commands into the frame being built. Handed to Layer::OnRender(), valid only during
// that call; recording happens inside the window target's render pass, with the viewport and scissor
// already covering the whole window (y up, like OpenGL).
class ICommandList
{
public:
    virtual ~ICommandList() = default;

    virtual void BindPipeline(const IPipeline& pipeline) = 0;
    virtual void BindVertexBuffer(const IVertexBuffer& buffer) = 0;
    virtual void BindIndexBuffer(const IIndexBuffer& buffer) = 0; // 32-bit indices

    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
};

class IWindowTarget : public RefCounted<IWindowTarget>
{
public:
    virtual ~IWindowTarget() = default;
    virtual void Destroy() = 0;

    // The window's framebuffer changed size: the swapchain is rebuilt before the next frame
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    // Waits for a free frame slot, acquires the next image and starts recording into it.
    // Returns false when there is nothing to render to this frame (e.g. a 0-sized window);
    // call EndFrame() only when BeginFrame() returned true.
    virtual bool BeginFrame() = 0;
    // Submits the frame's commands and presents the image
    virtual void EndFrame() = 0;

    // Valid between BeginFrame() and EndFrame()
    virtual ICommandList& GetCommandList() = 0;
    // The render pass frames are recorded in: pipelines drawing to the window must be created with it
    virtual RefCountPtr<IRenderPass> GetRenderPass() const = 0;
};

}
