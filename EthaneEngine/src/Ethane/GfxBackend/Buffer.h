//
//  Buffer.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2026/1/22.
//

#pragma once
#include "Resource.h"

namespace Ethane {

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
}
