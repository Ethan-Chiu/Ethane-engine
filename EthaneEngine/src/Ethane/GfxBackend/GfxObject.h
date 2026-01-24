//
//  GfxObject.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2026/1/24.
//

#pragma once
#include "Resource.h"

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
}
