#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Math/AABB.h"

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

    // A model file (anything assimp reads: fbx, obj, gltf, ...) uploaded to GPU buffers.
    // The CPU copy of the vertices is dropped after the upload.
    class Mesh
    {
    public:
        // Returns nullptr (and logs why) if the file can't be read or has no triangles
        static Ref<Mesh> Load(GfxBackendAPI& backend, const std::string& path);

        // Vertex input of MeshVertex, in order: a_Position, a_Normal, a_Tangent, a_Binormal, a_TexCoord
        // (locations 0..4). A pipeline drawing meshes uses this layout; its shader must declare all five.
        static const VertexBufferLayout& GetVertexLayout();

        // Binds the buffers and draws every node. Per node it pushes `model * node.Transform` as a mat4 at
        // push-constant offset 0, so the shader needs `layout(push_constant) uniform { mat4 model; ... }`.
        // The pipeline and binding sets must already be bound.
        void Draw(ICommandList& commandList, const glm::mat4& model = glm::mat4(1.0f)) const;

        const std::string& GetPath() const { return m_Path; }
        const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }
        const std::vector<MeshNode>& GetNodes() const { return m_Nodes; }
        const AABB& GetBoundingBox() const { return m_BoundingBox; } // all nodes, in mesh space
        uint32_t GetVertexCount() const { return m_VertexCount; }
        uint32_t GetIndexCount() const { return m_IndexCount; }
        const RefCountPtr<IVertexBuffer>& GetVertexBuffer() const { return m_VertexBuffer; }
        const RefCountPtr<IIndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }

    private:
        std::string m_Path;
        std::vector<Submesh> m_Submeshes;
        std::vector<MeshNode> m_Nodes;
        AABB m_BoundingBox;
        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;

        RefCountPtr<IVertexBuffer> m_VertexBuffer;
        RefCountPtr<IIndexBuffer> m_IndexBuffer;
    };

}
