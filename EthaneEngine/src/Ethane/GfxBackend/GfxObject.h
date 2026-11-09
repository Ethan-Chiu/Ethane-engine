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

// Host-visible buffer read by shaders. Backends keep one copy per frame in flight; SetData() writes the
// copy of the frame being recorded, so per-frame data should be written in Layer::OnRender(). Called
// outside a frame (e.g. OnAttach), it waits for the GPU to go idle and writes every copy.
class IUniformBuffer : public IBuffer {
public:
    virtual uint32_t GetSize() const = 0;
};

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

enum class TextureFilter { Linear, Nearest };
enum class TextureWrap { Repeat, ClampToEdge, MirroredRepeat };

struct TextureSpecification
{
    uint32_t Width = 1;
    uint32_t Height = 1;
    ImageFormat Format = ImageFormat::RGBA; // 8-bit UNORM RGBA, what ImageLoader produces
    TextureFilter Filter = TextureFilter::Linear;
    TextureWrap Wrap = TextureWrap::Repeat;
};

// Sampled 2D texture (one mip level). Row 0 of the pixel data is the image's top row, at v = 0.
class ITexture2D : public RefCounted<ITexture2D>
{
public:
    virtual ~ITexture2D() = default;
    virtual const TextureSpecification& GetSpecification() const = 0;
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
// The resources of one descriptor set of a shader: `layout(set = N, binding = B)`. Every binding the
// shader declares in that set must be filled before the set is bound. Bindings can be changed at any
// time; the backend applies them to a frame's copy only once the GPU is done with that frame.
class IBindingSet : public RefCounted<IBindingSet>
{
public:
    virtual ~IBindingSet() = default;
    virtual void SetUniformBuffer(uint32_t binding, const RefCountPtr<IUniformBuffer>& buffer) = 0;
    virtual void SetTexture(uint32_t binding, const RefCountPtr<ITexture2D>& texture) = 0;
};

class ICommandList
{
public:
    virtual ~ICommandList() = default;

    virtual void BindPipeline(const IPipeline& pipeline) = 0;
    virtual void BindVertexBuffer(const IVertexBuffer& buffer) = 0;
    virtual void BindIndexBuffer(const IIndexBuffer& buffer) = 0; // 32-bit indices
    // Binds to the set index the binding set was created for; needs a pipeline bound first (its layout is used)
    virtual void BindBindingSet(IBindingSet& bindingSet) = 0;

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
