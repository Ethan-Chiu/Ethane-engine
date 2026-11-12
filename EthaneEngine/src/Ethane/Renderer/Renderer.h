#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Renderer/FramePacket.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Ethane {

	class AssetLibrary;
	class GfxBackendAPI;
	class Material;
	class Mesh;

	struct RendererStats
	{
		uint32_t DrawItems = 0;
		uint32_t DrawCalls = 0;     // one per placed submesh
		uint32_t Triangles = 0;
		uint32_t MissingMeshes = 0; // draw items whose mesh couldn't be loaded
	};

	// Turns frame packets into GPU work. Owns the mesh shader and pipeline, the per-frame uniforms and
	// the GPU materials: each mesh's own, and those of elements with a material override (kept while the
	// element keeps being drawn, released a few frames after it stops).
	//
	// The pipeline is made for one render pass: Render into targets whose passes are compatible with it
	// (same formats), such as the target the renderer was created for, even after it's resized.
	class Renderer
	{
	public:
		// `meshShader`: a shader with the interface of the editor's assets/shaders/mesh.glsl
		Renderer(GfxBackendAPI& backend, AssetLibrary& assets, const RefCountPtr<IRenderPass>& renderPass, const std::string& meshShader);
		~Renderer();

		// Records the packet into `target`: begins and ends the target's render pass.
		// Call once per frame: the frame uniforms have one copy per frame in flight, and per-element
		// materials are released by counting calls.
		void Render(ICommandList& commandList, const FramePacket& packet, IRenderTarget& target);

		const RendererStats& GetStats() const { return m_Stats; } // of the last Render

	private:
		struct OverrideMaterials
		{
			AssetId Mesh;
			MaterialOverride Values;
			std::vector<Ref<Material>> Materials;
			uint64_t LastUsed = 0;
		};

		const std::vector<Ref<Material>>& MeshMaterials(AssetId id, const Mesh& mesh);
		const std::vector<Ref<Material>>& ElementMaterials(ElementId element, AssetId meshId, const Mesh& mesh, const MaterialOverride& values);
		void ReleaseUnused();

		GfxBackendAPI* m_Backend;
		AssetLibrary* m_Assets;
		RefCountPtr<IShader> m_MeshShader;
		RefCountPtr<IPipeline> m_MeshPipeline;
		RefCountPtr<IUniformBuffer> m_FrameUniforms;
		RefCountPtr<IBindingSet> m_FrameBindings;

		std::unordered_map<AssetId, std::vector<Ref<Material>>> m_MeshMaterials;
		std::unordered_map<ElementId, OverrideMaterials> m_ElementMaterials;
		uint64_t m_Frame = 0;
		RendererStats m_Stats;
	};

}
