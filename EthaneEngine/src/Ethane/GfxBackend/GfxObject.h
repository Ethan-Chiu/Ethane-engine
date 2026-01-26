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

class IWindowTarget : public RefCounted<IWindowTarget>
{
public:
    virtual ~IWindowTarget() = default;
    virtual void Destroy() = 0;
};

class IBuffer : public RefCounted<IBuffer>
{
public:
    virtual ~IBuffer() = default;
    virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
};

class IVertexBuffer : public IBuffer {};

class IIndexBuffer : public IBuffer {
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
    DEPTH24STENCIL8,
    // Defaults
    Depth = DEPTH24STENCIL8,
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
}
