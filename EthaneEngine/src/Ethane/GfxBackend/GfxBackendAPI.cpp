//
//  GfxBackendAPI.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//

#include "GfxBackendAPI.h"
#include "Vulkan/VulkanGfxBackendAPI.h"

namespace Ethane {

GfxBackendAPI::API GfxBackendAPI::s_API = GfxBackendAPI::API::Vulkan;

    Scope<GfxBackendAPI> GfxBackendAPI::Create()
    {
        switch (s_API)
        {
        case API::None:
            ETH_CORE_ASSERT(false, "rendererAPI::None is currently not supported!");
            return nullptr;
        case API::Vulkan:
            return CreateScope<VulkanGfxBackendAPI>();
        }

        ETH_CORE_ASSERT(false, "Unknown RendererAPI");
        return nullptr;
    }

}
