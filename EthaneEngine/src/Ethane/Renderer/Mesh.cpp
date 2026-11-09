#include "ethpch.h"
#include "Mesh.h"

#include "Ethane/GfxBackend/GfxBackendAPI.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

namespace Ethane {

    namespace {
        // Assimp matrices are row-major (a1..a4 is the first row), glm is column-major
        glm::mat4 ToGlm(const aiMatrix4x4& m)
        {
            return glm::mat4(
                m.a1, m.b1, m.c1, m.d1,
                m.a2, m.b2, m.c2, m.d2,
                m.a3, m.b3, m.c3, m.d3,
                m.a4, m.b4, m.c4, m.d4);
        }

        glm::vec3 ToGlm(const aiVector3D& v) { return { v.x, v.y, v.z }; }

        AABB EmptyBox()
        {
            // Grows from the first point; AABB's default (0, 0) would always contain the origin
            return AABB(glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::lowest()));
        }

        void Grow(AABB& box, const glm::vec3& point)
        {
            box.Min = glm::min(box.Min, point);
            box.Max = glm::max(box.Max, point);
        }

        // Assimp's output is already counter-clockwise, matching the engine's front faces (y-up viewport),
        // so no aiProcess_FlipWindingOrder.
        constexpr unsigned int kImportFlags =
            aiProcess_Triangulate |              // only triangles
            aiProcess_SortByPType |              // points / lines go to their own meshes, skipped below
            aiProcess_GenSmoothNormals |         // only if the file has none
            aiProcess_CalcTangentSpace |         // tangents / binormals, for normal mapping
            aiProcess_GenUVCoords |              // non-UV mappings (spherical, ...) to UVs
            aiProcess_JoinIdenticalVertices |    // indexed output
            aiProcess_ImproveCacheLocality |
            aiProcess_ValidateDataStructure;

        // Textures of one model file: loaded once even if several materials use them
        class TextureCache
        {
        public:
            TextureCache(GfxBackendAPI& backend, const aiScene* scene, const std::filesystem::path& modelPath)
                : m_Backend(backend), m_Scene(scene), m_Directory(modelPath.parent_path()) {}

            RefCountPtr<ITexture2D> Get(const aiMaterial* material, std::initializer_list<aiTextureType> types)
            {
                for (aiTextureType type : types) {
                    aiString path;
                    if (material->GetTextureCount(type) > 0 && material->GetTexture(type, 0, &path) == aiReturn_SUCCESS) {
                        return Load(path.C_Str());
                    }
                }
                return {};
            }

        private:
            RefCountPtr<ITexture2D> Load(const std::string& reference)
            {
                if (const auto it = m_Textures.find(reference); it != m_Textures.end()) {
                    return it->second;
                }

                RefCountPtr<ITexture2D> texture;
                // "*N" or, in fbx, the original file name of a texture stored inside the model
                if (const aiTexture* embedded = m_Scene->GetEmbeddedTexture(reference.c_str())) {
                    if (embedded->mHeight == 0) {
                        // compressed (png, jpg, ...): mWidth is the byte size
                        texture = m_Backend.LoadTexture2DFromMemory(embedded->pcData, embedded->mWidth);
                    } else {
                        // raw BGRA texels
                        std::vector<uint8_t> rgba(size_t(embedded->mWidth) * embedded->mHeight * 4);
                        for (size_t i = 0; i < size_t(embedded->mWidth) * embedded->mHeight; i++) {
                            const aiTexel& texel = embedded->pcData[i];
                            rgba[i * 4 + 0] = texel.r; rgba[i * 4 + 1] = texel.g; rgba[i * 4 + 2] = texel.b; rgba[i * 4 + 3] = texel.a;
                        }
                        texture = m_Backend.CreateTexture2D(TextureSpecification{ .Width = embedded->mWidth, .Height = embedded->mHeight }, rgba.data());
                    }
                } else {
                    std::string file = reference;
                    std::replace(file.begin(), file.end(), '\\', '/'); // paths written on Windows
                    const std::filesystem::path path = (m_Directory / file).lexically_normal();
                    texture = m_Backend.LoadTexture2D(path.string());
                }
                if (!texture) {
                    ETH_CORE_WARN("Mesh texture '{0}' couldn't be loaded, using the default", reference);
                }
                m_Textures.emplace(reference, texture);
                return texture;
            }

            GfxBackendAPI& m_Backend;
            const aiScene* m_Scene;
            std::filesystem::path m_Directory;
            std::unordered_map<std::string, RefCountPtr<ITexture2D>> m_Textures;
        };

        MaterialData ImportMaterial(const aiMaterial* source, TextureCache& textures)
        {
            MaterialData material;
            material.Name = source->GetName().C_Str();

            // PBR formats (gltf, ...) give a base color; older ones a diffuse color and an opacity
            aiColor4D color;
            if (source->Get(AI_MATKEY_BASE_COLOR, color) == aiReturn_SUCCESS) {
                material.BaseColor = { color.r, color.g, color.b, color.a };
            } else if (source->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS) {
                float opacity = 1.0f;
                source->Get(AI_MATKEY_OPACITY, opacity);
                material.BaseColor = { color.r, color.g, color.b, opacity };
            }

            float value = 0.0f;
            if (source->Get(AI_MATKEY_METALLIC_FACTOR, value) == aiReturn_SUCCESS) {
                material.Metallic = value;
            }
            if (source->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == aiReturn_SUCCESS) {
                material.Roughness = value;
            } else if (source->Get(AI_MATKEY_SHININESS, value) == aiReturn_SUCCESS && value > 0.0f) {
                // Blinn-Phong exponent -> roughness (inverse of exponent = 2 / roughness^4 - 2), in [0.05, 1]
                material.Roughness = glm::clamp(std::pow(2.0f / (value + 2.0f), 0.25f), 0.05f, 1.0f);
            }

            material.BaseColorTexture = textures.Get(source, { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE });
            material.NormalTexture = textures.Get(source, { aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA });
            material.MetallicRoughnessTexture = textures.Get(source, { aiTextureType_GLTF_METALLIC_ROUGHNESS });
            return material;
        }

        void CollectNodes(const aiNode* node, const glm::mat4& parentTransform, const std::vector<int32_t>& submeshOfMesh, std::vector<MeshNode>& nodes)
        {
            const glm::mat4 transform = parentTransform * ToGlm(node->mTransformation);
            for (uint32_t i = 0; i < node->mNumMeshes; i++) {
                const int32_t submesh = submeshOfMesh[node->mMeshes[i]];
                if (submesh >= 0) {
                    nodes.push_back(MeshNode{ .SubmeshIndex = static_cast<uint32_t>(submesh), .Transform = transform, .Name = node->mName.C_Str() });
                }
            }
            for (uint32_t i = 0; i < node->mNumChildren; i++) {
                CollectNodes(node->mChildren[i], transform, submeshOfMesh, nodes);
            }
        }
    }

    const VertexBufferLayout& Mesh::GetVertexLayout()
    {
        static const VertexBufferLayout layout = {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Normal" },
            { ShaderDataType::Float3, "a_Tangent" },
            { ShaderDataType::Float3, "a_Binormal" },
            { ShaderDataType::Float2, "a_TexCoord" },
        };
        return layout;
    }

    Ref<Mesh> Mesh::Load(GfxBackendAPI& backend, const std::string& path)
    {
        ETH_CORE_INFO("Loading mesh: {0}", path);

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, kImportFlags);
        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            ETH_CORE_ERROR("Failed to load mesh {0}: {1}", path, importer.GetErrorString());
            return nullptr;
        }

        auto mesh = CreateRef<Mesh>();
        mesh->m_Path = path;

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<int32_t> submeshOfMesh(scene->mNumMeshes, -1); // assimp mesh -> submesh (-1: skipped)

        for (uint32_t m = 0; m < scene->mNumMeshes; m++)
        {
            const aiMesh* source = scene->mMeshes[m];
            if (!(source->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) || !source->HasPositions() || !source->HasNormals()) {
                ETH_CORE_WARN("Mesh {0}: skipping '{1}' (no triangles)", path, source->mName.C_Str());
                continue;
            }

            Submesh submesh{
                .BaseVertex    = static_cast<uint32_t>(vertices.size()),
                .BaseIndex     = static_cast<uint32_t>(indices.size()),
                .VertexCount   = source->mNumVertices,
                .IndexCount    = source->mNumFaces * 3,
                .MaterialIndex = source->mMaterialIndex,
                .BoundingBox   = EmptyBox(),
                .Name          = source->mName.C_Str()};

            const bool hasTangents = source->HasTangentsAndBitangents();
            const bool hasTexCoords = source->HasTextureCoords(0);
            for (uint32_t i = 0; i < source->mNumVertices; i++)
            {
                MeshVertex& vertex = vertices.emplace_back();
                vertex.Position = ToGlm(source->mVertices[i]);
                vertex.Normal = ToGlm(source->mNormals[i]);
                vertex.Tangent = hasTangents ? ToGlm(source->mTangents[i]) : glm::vec3(0.0f);
                vertex.Binormal = hasTangents ? ToGlm(source->mBitangents[i]) : glm::vec3(0.0f);
                // Files use v = 0 at the bottom of the image (OpenGL convention); textures are loaded top row first
                vertex.TexCoord = hasTexCoords ? glm::vec2(source->mTextureCoords[0][i].x, 1.0f - source->mTextureCoords[0][i].y) : glm::vec2(0.0f);
                Grow(submesh.BoundingBox, vertex.Position);
            }

            // Indices stay relative to the submesh: drawn with vertexOffset = BaseVertex
            for (uint32_t i = 0; i < source->mNumFaces; i++)
            {
                const aiFace& face = source->mFaces[i];
                ETH_CORE_ASSERT(face.mNumIndices == 3, "Triangulated mesh has a non-triangle face");
                indices.insert(indices.end(), face.mIndices, face.mIndices + 3);
            }

            submeshOfMesh[m] = static_cast<int32_t>(mesh->m_Submeshes.size());
            mesh->m_Submeshes.push_back(std::move(submesh));
        }

        if (mesh->m_Submeshes.empty()) {
            ETH_CORE_ERROR("Failed to load mesh {0}: no triangle meshes", path);
            return nullptr;
        }

        CollectNodes(scene->mRootNode, glm::mat4(1.0f), submeshOfMesh, mesh->m_Nodes);

        TextureCache textures(backend, scene, std::filesystem::path(path));
        mesh->m_Materials.reserve(scene->mNumMaterials);
        for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
            mesh->m_Materials.push_back(ImportMaterial(scene->mMaterials[i], textures));
        }
        if (mesh->m_Materials.empty()) {
            mesh->m_Materials.emplace_back(); // MaterialIndex 0 must exist
        }

        // Bounds of every placed submesh: transform the 8 corners of its box
        mesh->m_BoundingBox = EmptyBox();
        for (const MeshNode& node : mesh->m_Nodes) {
            const AABB& box = mesh->m_Submeshes[node.SubmeshIndex].BoundingBox;
            for (uint32_t corner = 0; corner < 8; corner++) {
                const glm::vec3 point{ (corner & 1) ? box.Max.x : box.Min.x, (corner & 2) ? box.Max.y : box.Min.y, (corner & 4) ? box.Max.z : box.Min.z };
                Grow(mesh->m_BoundingBox, glm::vec3(node.Transform * glm::vec4(point, 1.0f)));
            }
        }

        mesh->m_VertexCount = static_cast<uint32_t>(vertices.size());
        mesh->m_IndexCount = static_cast<uint32_t>(indices.size());

        const uint32_t vertexBytes = static_cast<uint32_t>(vertices.size() * sizeof(MeshVertex));
        const uint32_t indexBytes = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
        mesh->m_VertexBuffer = backend.CreateVertexBuffer(vertexBytes);
        mesh->m_VertexBuffer->SetData(vertices.data(), vertexBytes);
        mesh->m_IndexBuffer = backend.CreateIndexBuffer(indexBytes);
        mesh->m_IndexBuffer->SetData(indices.data(), indexBytes);

        const AABB& bounds = mesh->m_BoundingBox;
        ETH_CORE_INFO("Mesh {0}: {1} submeshes, {2} nodes, {3} materials, {4} vertices, {5} triangles, bounds ({6}, {7}, {8}) - ({9}, {10}, {11})",
                      path, mesh->m_Submeshes.size(), mesh->m_Nodes.size(), mesh->m_Materials.size(), mesh->m_VertexCount, mesh->m_IndexCount / 3,
                      bounds.Min.x, bounds.Min.y, bounds.Min.z, bounds.Max.x, bounds.Max.y, bounds.Max.z);
        return mesh;
    }

    void Mesh::Draw(ICommandList& commandList, const glm::mat4& model, std::span<const Ref<Material>> materials) const
    {
        commandList.BindVertexBuffer(*m_VertexBuffer);
        commandList.BindIndexBuffer(*m_IndexBuffer);
        const Material* bound = nullptr;
        for (const MeshNode& node : m_Nodes)
        {
            const Submesh& submesh = m_Submeshes[node.SubmeshIndex];
            if (!materials.empty()) {
                const Material* material = (submesh.MaterialIndex < materials.size() ? materials[submesh.MaterialIndex] : materials[0]).get();
                if (material != bound) {
                    material->Bind(commandList);
                    bound = material;
                }
            }
            const glm::mat4 transform = model * node.Transform;
            commandList.PushConstants(&transform, sizeof(transform));
            commandList.DrawIndexed(submesh.IndexCount, 1, submesh.BaseIndex, static_cast<int32_t>(submesh.BaseVertex));
        }
    }

}
