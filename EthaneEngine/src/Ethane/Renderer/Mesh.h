#pragma once

#include "Ethane/Asset/ImageLoader.h"
#include "Ethane/Core/Base.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Math/AABB.h"
#include "Material.h"

#include <span>

#include <glm/glm.hpp>

namespace Ethane {

    class GfxBackendAPI;

    // Vertex format of every Mesh, see Mesh::GetVertexLayout() for the matching shader inputs
    struct MeshVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec3 Binormal;
        glm::vec2 TexCoord;
    };

    // A range of the mesh's shared vertex / index buffers: one assimp mesh (one material)
    struct Submesh
    {
        uint32_t BaseVertex = 0;
        uint32_t BaseIndex = 0;
        uint32_t VertexCount = 0;
        uint32_t IndexCount = 0;
        uint32_t MaterialIndex = 0;
        AABB BoundingBox; // in the submesh's own space
        std::string Name;
    };

    // Where the file's node hierarchy places a submesh. A submesh can be placed by several nodes.
    struct MeshNode
    {
        uint32_t SubmeshIndex = 0;
        glm::mat4 Transform{ 1.0f }; // node -> mesh space, parents applied
        std::string Name;
    };

    // A model file read into memory, its textures decoded: everything Mesh::Create needs, made without the
    // GPU, so on any thread (Mesh::Import)
    struct MeshImport
    {
        struct Material
        {
            MaterialData Factors; // its texture members are left empty: the images below become them
            std::shared_ptr<const DecodedImage> BaseColor;
            std::shared_ptr<const DecodedImage> Normal;
            std::shared_ptr<const DecodedImage> MetallicRoughness;
        };

        std::string Path;
        std::vector<MeshVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::vector<Submesh> Submeshes;
        std::vector<MeshNode> Nodes;
        std::vector<Material> Materials; // by Submesh::MaterialIndex; at least one
        AABB BoundingBox;
    };

    // A model file (anything assimp reads: fbx, obj, gltf, ...) uploaded to GPU buffers, with the file's
    // materials (factors + textures, loaded relative to the file or from embedded data).
    // The CPU copy of the vertices is dropped after the upload.
    class Mesh
    {
    public:
        // Reads the file and decodes its textures; no GPU work, safe on any thread. nullopt (and logs why) if
        // the file can't be read or has no triangles.
        static std::optional<MeshImport> Import(const std::string& path);
        // Uploads an import (on the thread that creates GPU resources); images shared by several materials
        // become one texture
        static Ref<Mesh> Create(GfxBackendAPI& backend, const MeshImport& meshImport);
        // Import, then Create; nullptr if the import fails
        static Ref<Mesh> Load(GfxBackendAPI& backend, const std::string& path);

        // Vertex input of MeshVertex, in order: a_Position, a_Normal, a_Tangent, a_Binormal, a_TexCoord
        // (locations 0..4). A pipeline drawing meshes uses this layout; its shader must declare all five.
        static const VertexBufferLayout& GetVertexLayout();

        // Binds the buffers and draws every node. Per node it pushes `model * node.Transform` as a mat4 at
        // push-constant offset 0, so the shader needs `layout(push_constant) uniform { mat4 model; ... }`.
        // Each submesh binds materials[MaterialIndex] (e.g. from Material::CreateAll(GetMaterials()));
        // an index past the end uses materials[0], so a single material covers the whole mesh. With no
        // materials nothing is bound. The pipeline and the other binding sets must already be bound.
        void Draw(ICommandList& commandList, const glm::mat4& model = glm::mat4(1.0f), std::span<const Ref<Material>> materials = {}) const;

        const std::string& GetPath() const { return m_Path; }
        const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }
        const std::vector<MeshNode>& GetNodes() const { return m_Nodes; }
        const std::vector<MaterialData>& GetMaterials() const { return m_Materials; } // by Submesh::MaterialIndex
        const AABB& GetBoundingBox() const { return m_BoundingBox; } // all nodes, in mesh space
        uint32_t GetVertexCount() const { return m_VertexCount; }
        uint32_t GetIndexCount() const { return m_IndexCount; }
        const RefCountPtr<IVertexBuffer>& GetVertexBuffer() const { return m_VertexBuffer; }
        const RefCountPtr<IIndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }

    private:
        std::string m_Path;
        std::vector<Submesh> m_Submeshes;
        std::vector<MeshNode> m_Nodes;
        std::vector<MaterialData> m_Materials;
        AABB m_BoundingBox;
        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;

        RefCountPtr<IVertexBuffer> m_VertexBuffer;
        RefCountPtr<IIndexBuffer> m_IndexBuffer;
    };

}
