#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Renderer/FramePacket.h"

#include <glm/glm.hpp>

#include <mutex>
#include <optional>
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
		uint32_t MissingMeshes = 0; // draw items whose mesh isn't available: still loading, or failed
	};

	// Turns frame packets into GPU work. Render and the GPU objects it owns belong to the recording thread;
	// GetStats can be called from anywhere. Owns the mesh shader and pipeline, the per-frame uniforms and
	// the GPU materials: each mesh's own, and those of elements with material params (kept while the
	// element keeps being drawn, released when a frame doesn't draw it).
	//
	// The pipeline is made for one render pass: Render into targets whose passes are compatible with it
	// (same formats), such as the target the renderer was created for, even after it's resized.
	class Renderer
	{
	public:
		// `meshShader` / `pickShader`: shaders with the interfaces of the editor's assets/shaders/mesh.glsl and
		// pick.glsl
		Renderer(GfxBackendAPI& backend, AssetLibrary& assets, const RefCountPtr<IRenderPass>& renderPass,
		         const std::string& meshShader, const std::string& pickShader);
		~Renderer();

		// Records the packet into `target`: begins and ends the target's render pass.
		// Call once per frame: the frame uniforms have one copy per frame in flight, and per-element
		// materials not drawn by this call are released.
		void Render(ICommandList& commandList, const FramePacket& packet, IRenderTarget& target);

		// Picking: asks which element covers `pixel` ((0, 0): top left) of the target the next Render draws
		// into. That Render also draws the packet into a 1x1 id image and copies its pixel back; the answer
		// comes a frame or two later, once the GPU has finished that frame. A newer request replaces an older
		// one. Both can be called from any thread.
		void RequestPick(glm::uvec2 pixel);
		// The answer to the latest request, once: the element drawn at the pixel (a null id: nothing there, or
		// a draw item that wasn't from a stele), or nullopt while no answer is ready
		std::optional<ElementId> TakePick();

		// Of the last Render: a copy, safe to read from another thread than the one recording
		RendererStats GetStats() const
		{
			std::lock_guard lock(m_StatsMutex);
			return m_PublishedStats;
		}

	private:
		struct ElementMaterialSet
		{
			AssetId Mesh;
			MaterialParams Values;
			RefCountPtr<ITexture2D> Texture; // the base color texture applied (null: not loaded yet, or none)
			std::vector<Ref<Material>> Materials;
			uint64_t LastUsed = 0;
		};

		struct PendingPick
		{
			RefCountPtr<IReadbackBuffer> Readback;
			std::vector<ElementId> Elements; // by pick id - 1 (the draw item's index)
		};
		void RecordPick(ICommandList& commandList, const FramePacket& packet, const IRenderTarget& target, glm::uvec2 pixel);
		void CollectPick();

		const std::vector<Ref<Material>>& MeshMaterials(AssetId id, const Mesh& mesh);
		const std::vector<Ref<Material>>& ElementMaterials(ElementId element, AssetId meshId, const Mesh& mesh, const MaterialParams& values);
		void ReleaseUnused();

		GfxBackendAPI* m_Backend;
		AssetLibrary* m_Assets;
		RefCountPtr<IShader> m_MeshShader;
		RefCountPtr<IPipeline> m_MeshPipeline;
		RefCountPtr<IUniformBuffer> m_FrameUniforms;
		RefCountPtr<IBindingSet> m_FrameBindings;

		// Picking: a 1x1 R32UI (+ depth) target and the pipeline drawing ids into it
		RefCountPtr<IRenderTarget> m_PickTarget;
		RefCountPtr<IShader> m_PickShader;
		RefCountPtr<IPipeline> m_PickPipeline;
		RefCountPtr<IUniformBuffer> m_PickUniforms;
		RefCountPtr<IBindingSet> m_PickBindings;
		std::optional<PendingPick> m_PendingPick; // recording side
		std::mutex m_PickMutex;                   // guards the two below
		std::optional<glm::uvec2> m_PickRequest;
		std::optional<ElementId> m_PickResult;

		std::unordered_map<AssetId, std::vector<Ref<Material>>> m_MeshMaterials;
		std::unordered_map<ElementId, ElementMaterialSet> m_ElementMaterials;
		uint64_t m_Frame = 0;
		RendererStats m_Stats;           // being counted by Render
		RendererStats m_PublishedStats;  // the last complete count
		mutable std::mutex m_StatsMutex;
	};

}
