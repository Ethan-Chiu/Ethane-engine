#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/GfxBackend/GfxObject.h"

#include <glm/glm.hpp>

namespace Ethane {

    class GfxBackendAPI;

    // What a surface looks like, independent of any shader. Missing textures fall back to defaults that
    // leave the factors unchanged (white, flat normal). Metallic-roughness follows glTF: G = roughness,
    // B = metallic, each multiplied by its factor.
    struct MaterialData
    {
        std::string Name;
        glm::vec4 BaseColor{ 1.0f };
        float Metallic = 0.0f;
        float Roughness = 0.8f;

        RefCountPtr<ITexture2D> BaseColorTexture;
        RefCountPtr<ITexture2D> NormalTexture;            // tangent space, OpenGL convention (+Y up the image)
        RefCountPtr<ITexture2D> MetallicRoughnessTexture;
    };

    // A MaterialData bound to a shader's material descriptor set (set 1):
    //   binding 0: uniform MaterialUniforms { vec4 BaseColor; float Metallic; float Roughness; }
    //   binding 1: sampler2D u_BaseColorMap
    //   binding 2: sampler2D u_NormalMap
    //   binding 3: sampler2D u_MetallicRoughnessMap
    // See assets/shaders/mesh.glsl in the editor.
    class Material
    {
    public:
        static constexpr uint32_t kDescriptorSet = 1;

        Material(GfxBackendAPI& backend, const RefCountPtr<IShader>& shader, const MaterialData& data);

        // One material per entry, e.g. for Mesh::GetMaterials()
        static std::vector<Ref<Material>> CreateAll(GfxBackendAPI& backend, const RefCountPtr<IShader>& shader, const std::vector<MaterialData>& materials);

        // Can be called at any time; takes effect for frames recorded afterwards
        void SetData(const MaterialData& data);
        const MaterialData& GetData() const { return m_Data; }

        // Binds set 1 (a pipeline using the material's shader must be bound). Writes the parameters into
        // the frame's copy of the uniform buffer on every bind: it's a few bytes, and it keeps every frame
        // in flight up to date without tracking which copies are stale.
        void Bind(ICommandList& commandList) const;

    private:
        void ApplyTextures();

    private:
        GfxBackendAPI* m_Backend = nullptr; // for the default textures
        MaterialData m_Data;
        RefCountPtr<IUniformBuffer> m_Uniforms;
        RefCountPtr<IBindingSet> m_BindingSet;
    };

}
