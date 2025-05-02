//
//  WindowTarget.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/1.
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
}
