//
//  VulkanBindingSet.cpp
//  EthaneEngine
//
#include "ethpch.h"
#include "VulkanBindingSet.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

namespace Ethane {

    VulkanBindingSet::VulkanBindingSet(const VulkanDevice* device, const RefCountPtr<VulkanShader>& shader, uint32_t set)
        : m_Device(device), m_Shader(shader), m_Set(set)
    {
        ETH_CORE_ASSERT(m_Shader, "Binding set needs a shader");
        ETH_CORE_ASSERT(set < m_Shader->GetShaderDescriptorSetData().size(), "Shader has no such descriptor set");

        const auto& reflection = Reflection();
        if (!reflection.StorageBuffers.empty() || !reflection.StorageImage.empty()) {
            ETH_CORE_ERROR("Binding set {0} of {1}: storage buffers/images aren't supported yet", set, m_Shader->GetName());
        }

        m_Pool = m_Shader->CreateDescriptorPool(VulkanDevice::kMaxFramesInFlight);
        m_Sets.reserve(VulkanDevice::kMaxFramesInFlight);
        for (uint32_t i = 0; i < VulkanDevice::kMaxFramesInFlight; i++) {
            m_Sets.push_back(m_Shader->CreateDescriptorSet(set, m_Pool));
        }
    }

    const VulkanShaderCompiler::ShaderDescriptorSetData& VulkanBindingSet::Reflection() const
    {
        return m_Shader->GetShaderDescriptorSetData()[m_Set];
    }

    void VulkanBindingSet::SetUniformBuffer(uint32_t binding, const RefCountPtr<IUniformBuffer>& buffer)
    {
        const auto& uniformBuffers = Reflection().UniformBuffers;
        const auto it = uniformBuffers.find(binding);
        if (it == uniformBuffers.end()) {
            ETH_CORE_ERROR("{0}: set {1} has no uniform buffer at binding {2}", m_Shader->GetName(), m_Set, binding);
            ETH_CORE_ASSERT(false, "No uniform buffer at this binding");
            return;
        }
        ETH_CORE_ASSERT(!buffer || buffer->GetSize() >= it->second.Size, "Uniform buffer is smaller than the shader's block");

        m_Resources[binding] = Resource{ .UniformBuffer = buffer };
        m_Version++;
    }

    void VulkanBindingSet::SetTexture(uint32_t binding, const RefCountPtr<ITexture2D>& texture)
    {
        const auto& samplers = Reflection().ImageSamplers;
        const auto it = samplers.find(binding);
        if (it == samplers.end()) {
            ETH_CORE_ERROR("{0}: set {1} has no sampler at binding {2}", m_Shader->GetName(), m_Set, binding);
            ETH_CORE_ASSERT(false, "No sampler at this binding");
            return;
        }
        ETH_CORE_ASSERT(it->second.ArraySize == 1, "Sampler arrays aren't supported yet");

        m_Resources[binding] = Resource{ .Texture = texture };
        m_Version++;
    }

    bool VulkanBindingSet::IsComplete() const
    {
        bool complete = true;
        auto check = [&](uint32_t binding, const std::string& name, bool isTexture) {
            const auto it = m_Resources.find(binding);
            const bool set = it != m_Resources.end() && (isTexture ? static_cast<bool>(it->second.Texture) : static_cast<bool>(it->second.UniformBuffer));
            if (!set) {
                ETH_CORE_ERROR("{0}: set {1} binding {2} ('{3}') has no resource", m_Shader->GetName(), m_Set, binding, name);
                complete = false;
            }
        };
        for (const auto& [binding, ubo] : Reflection().UniformBuffers) check(binding, ubo.Name, false);
        for (const auto& [binding, sampler] : Reflection().ImageSamplers) check(binding, sampler.Name, true);
        return complete;
    }

    void VulkanBindingSet::WriteFrame(uint32_t frameIndex)
    {
        const vk::DescriptorSet set = *m_Sets[frameIndex];

        // reserve: writes point into these vectors
        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::DescriptorImageInfo> imageInfos;
        bufferInfos.reserve(m_Resources.size());
        imageInfos.reserve(m_Resources.size());
        std::vector<vk::WriteDescriptorSet> writes;

        for (const auto& [binding, resource] : m_Resources)
        {
            if (resource.UniformBuffer)
            {
                const auto& ubo = static_cast<const VulkanUniformBuffer&>(*resource.UniformBuffer);
                bufferInfos.push_back(vk::DescriptorBufferInfo{ .buffer = ubo.GetHandle(frameIndex), .offset = 0, .range = ubo.GetSize() });
                writes.push_back(vk::WriteDescriptorSet{
                    .dstSet          = set,
                    .dstBinding      = binding,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType  = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo     = &bufferInfos.back()});
            }
            else if (resource.Texture)
            {
                imageInfos.push_back(static_cast<const VulkanTexture2D&>(*resource.Texture).GetDescriptorInfo());
                writes.push_back(vk::WriteDescriptorSet{
                    .dstSet          = set,
                    .dstBinding      = binding,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo      = &imageInfos.back()});
            }
        }

        m_Device->Get().updateDescriptorSets(writes, {});
        // the copy now references these: keep them alive until it's rewritten (by then its frame is done)
        m_FrameResources[frameIndex] = m_Resources;
        m_FrameVersion[frameIndex] = m_Version;
    }

    vk::DescriptorSet VulkanBindingSet::Prepare()
    {
        ETH_CORE_ASSERT(m_Device->IsFrameInProgress(), "Binding sets can only be bound while recording a frame");
        const uint32_t frame = m_Device->GetFrameIndex();
        const uint64_t serial = m_Device->GetFrameSerial();

        if (m_FrameVersion[frame] != m_Version)
        {
            if (m_FrameBoundSerial[frame] == serial) {
                // Already bound earlier in this frame: rewriting it now would invalidate the commands recorded
                // with it. Keep the old contents; the change lands the next time this copy is used.
                ETH_CORE_WARN("{0}: binding set {1} changed after being bound this frame, applied next frame", m_Shader->GetName(), m_Set);
            }
            else if (IsComplete()) {
                WriteFrame(frame);
            }
            else {
                ETH_CORE_ASSERT(m_FrameVersion[frame] != 0, "Binding set bound before all its bindings were set");
            }
        }

        m_FrameBoundSerial[frame] = serial;
        return *m_Sets[frame];
    }

}
