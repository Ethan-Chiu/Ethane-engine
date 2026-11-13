//
//  VulkanBindingSet.h
//  EthaneEngine
//
#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "VulkanContext.h"
#include "VulkanShader.h"

namespace Ethane {

    // One descriptor set of a shader, with one copy per frame in flight. Set*() only records the change;
    // Prepare() writes it into the current frame's copy, which is safe because the window target waited
    // for that frame's fence. Each copy keeps the resources it references alive (m_FrameResources), so a
    // replaced texture/buffer survives until the frames still reading it are done.
    class VulkanBindingSet : public IBindingSet
    {
    public:
        VulkanBindingSet(const VulkanDevice* device, const RefCountPtr<VulkanShader>& shader, uint32_t set);
        // Frames in flight may still have its sets bound: freed (sets, then pool) once they've finished
        virtual ~VulkanBindingSet() { m_Device->Retire(std::move(m_Sets), std::move(m_Pool)); }

        virtual void SetUniformBuffer(uint32_t binding, const RefCountPtr<IUniformBuffer>& buffer) override;
        virtual void SetTexture(uint32_t binding, const RefCountPtr<ITexture2D>& texture) override;

        // While recording a frame: brings this frame's copy up to date and returns it
        vk::DescriptorSet Prepare();
        uint32_t GetSetIndex() const { return m_Set; }

    private:
        struct Resource
        {
            RefCountPtr<IUniformBuffer> UniformBuffer;
            RefCountPtr<ITexture2D> Texture;
        };
        using ResourceMap = std::map<uint32_t, Resource>; // binding -> resource

        const VulkanShaderCompiler::ShaderDescriptorSetData& Reflection() const;
        bool IsComplete() const;
        void WriteFrame(uint32_t frameIndex);

    private:
        const VulkanDevice* m_Device = nullptr;
        VulkanLiveResource m_LiveResource;
        RefCountPtr<VulkanShader> m_Shader; // owns the set layout
        uint32_t m_Set = 0;

        ResourceMap m_Resources;                                                  // latest bindings
        uint64_t m_Version = 1;                                                   // bumped by every Set*()
        std::array<uint64_t, VulkanDevice::kMaxFramesInFlight> m_FrameVersion{};  // version each copy holds (0: never written)
        std::array<uint64_t, VulkanDevice::kMaxFramesInFlight> m_FrameBoundSerial{}; // frame serial the copy was last bound in
        std::array<ResourceMap, VulkanDevice::kMaxFramesInFlight> m_FrameResources; // what each copy references

        // NOTE: declaration order matters, sets are freed before their pool is destroyed
        vk::raii::DescriptorPool m_Pool = nullptr;
        std::vector<vk::raii::DescriptorSet> m_Sets; // one per frame in flight
    };

}
