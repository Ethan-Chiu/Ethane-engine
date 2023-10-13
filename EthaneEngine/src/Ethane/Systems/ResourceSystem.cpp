//
//  ResourceSystem.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2023/5/23.
//

#include "ethpch.h"
#include "ResourceSystem.h"

namespace Ethane {

    ImageLoader ResourceSystem::s_ImageLoader = ImageLoader();

    bool ResourceSystem::Init()
    {
        return true;
    }

    void ResourceSystem::Shutdown()
    {
        
    }
    
    bool ResourceSystem::LoadImage(const std::string& filepath, ImageResource& out_resource)
    {
        if(!s_ImageLoader.LoadImage(filepath, out_resource))
        {
            ETH_CORE_WARN("Fail to load image at {0}", filepath);
            return false;
        }
        return true;
    }

    void ResourceSystem::UnloadImage(ImageResource& resource)
    {
        s_ImageLoader.UnloadImage(resource);
    }
}
