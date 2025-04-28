//
//  GfxBackendAPI.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//
#pragma once

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
