#pragma once

#include "Ethane.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Renderer/EditorCamera.h"
#include "Ethane/Renderer/Mesh.h"

namespace Ethane {

	// The default meshes (res/meshes/default/*.fbx) lined up on a textured, normal-mapped floor
	// (assets/models/CobblestoneFloor), drawn with assets/shaders/mesh.glsl (set 0: camera, set 1: Material).
	// A stand-in for a real Scene until entities carry meshes.
	class DemoScene
	{
	public:
		struct Model
		{
			std::string Name;
			Ref<Mesh> MeshData;
			glm::mat4 Normalize{ 1.0f }; // fits the mesh in a unit box resting on y = 0 (the floor: 12 x 12)
			glm::vec3 Position{ 0.0f };
			float Scale = 1.0f;
			std::vector<Ref<Material>> Materials; // by submesh material index
			bool Spin = true;
			bool Visible = true;
		};

		// `renderPass`: what the scene will be drawn in (its pipeline is created for it)
		DemoScene(GfxBackendAPI& backend, const RefCountPtr<IRenderPass>& renderPass);

		void OnUpdate(Timestep ts);
		// Inside a render pass compatible with the one given at creation
		void Render(ICommandList& commandList, const EditorCamera& camera);

		std::vector<Model>& GetModels() { return m_Models; }
		float SpinSpeed = 0.5f; // radians per second

	private:
		std::vector<Model> m_Models;
		RefCountPtr<IShader> m_Shader;
		RefCountPtr<IPipeline> m_Pipeline;
		RefCountPtr<IUniformBuffer> m_CameraBuffer;
		RefCountPtr<IBindingSet> m_BindingSet;
		float m_Time = 0.0f;
	};

}
