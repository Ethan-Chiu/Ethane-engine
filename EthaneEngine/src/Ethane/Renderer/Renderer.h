#pragma once

#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Renderer/FramePacket.h"

#include <string>

namespace Ethane {

	class GfxBackendAPI;

	struct RendererStats
	{
		uint32_t DrawItems = 0;
		uint32_t DrawCalls = 0; // one per placed submesh
		uint32_t Triangles = 0;
	};

	// Turns frame packets into GPU work. Owns the mesh shader and pipeline and the per-frame uniforms.
	//
	// The pipeline is made for one render pass: Render into targets whose passes are compatible with it
	// (same formats), such as the target the renderer was created for, even after it's resized.
	// Materials drawn by the renderer must be created for GetMeshShader().
	class Renderer
	{
	public:
		// `meshShader`: a shader with the interface of the editor's assets/shaders/mesh.glsl
		Renderer(GfxBackendAPI& backend, const RefCountPtr<IRenderPass>& renderPass, const std::string& meshShader);

		// Records the packet into `target`: begins and ends the target's render pass.
		// Call at most once per frame: the frame uniforms have one copy per frame in flight.
		void Render(ICommandList& commandList, const FramePacket& packet, IRenderTarget& target);

		const RefCountPtr<IShader>& GetMeshShader() const { return m_MeshShader; }
		const RendererStats& GetStats() const { return m_Stats; } // of the last Render

	private:
		RefCountPtr<IShader> m_MeshShader;
		RefCountPtr<IPipeline> m_MeshPipeline;
		RefCountPtr<IUniformBuffer> m_FrameUniforms;
		RefCountPtr<IBindingSet> m_FrameBindings;
		RendererStats m_Stats;
	};

}
