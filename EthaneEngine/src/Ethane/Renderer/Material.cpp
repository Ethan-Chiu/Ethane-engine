#include "ethpch.h"
#include "Material.h"

#include "Ethane/GfxBackend/GfxBackendAPI.h"

namespace Ethane {

    namespace {
        // std140 layout of MaterialUniforms
        struct MaterialUniforms
        {
            glm::vec4 BaseColor;
            float Metallic;
            float Roughness;
            float Padding[2];
        };
        static_assert(sizeof(MaterialUniforms) == 32);
    }

    Material::Material(GfxBackendAPI& backend, const RefCountPtr<IShader>& shader, const MaterialData& data)
        : m_Backend(&backend), m_Data(data)
    {
        m_Uniforms = backend.CreateUniformBuffer(sizeof(MaterialUniforms));
        m_BindingSet = backend.CreateBindingSet(shader, kDescriptorSet);
        m_BindingSet->SetUniformBuffer(0, m_Uniforms);
        ApplyTextures();
    }

    std::vector<Ref<Material>> Material::CreateAll(GfxBackendAPI& backend, const RefCountPtr<IShader>& shader, const std::vector<MaterialData>& materials)
    {
        std::vector<Ref<Material>> result;
        result.reserve(materials.size());
        for (const MaterialData& data : materials) {
            result.push_back(CreateRef<Material>(backend, shader, data));
        }
        return result;
    }

    void Material::SetData(const MaterialData& data)
    {
        m_Data = data;
        ApplyTextures(); // binding sets apply changes per frame, once that frame's GPU work is done
    }

    void Material::ApplyTextures()
    {
        using Default = GfxBackendAPI::DefaultTexture;
        auto orDefault = [&](const RefCountPtr<ITexture2D>& texture, Default fallback) {
            return texture ? texture : m_Backend->GetDefaultTexture(fallback);
        };
        m_BindingSet->SetTexture(1, orDefault(m_Data.BaseColorTexture, Default::White));
        m_BindingSet->SetTexture(2, orDefault(m_Data.NormalTexture, Default::FlatNormal));
        m_BindingSet->SetTexture(3, orDefault(m_Data.MetallicRoughnessTexture, Default::White));
    }

    void Material::Bind(ICommandList& commandList) const
    {
        const MaterialUniforms uniforms{
            .BaseColor = m_Data.BaseColor,
            .Metallic  = m_Data.Metallic,
            .Roughness = m_Data.Roughness,
            .Padding   = {}};
        m_Uniforms->SetData(&uniforms, sizeof(uniforms));
        commandList.BindBindingSet(*m_BindingSet);
    }

}
