#pragma once

#include "Ethane.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Renderer/EditorCamera.h"
#include "Ethane/Renderer/Mesh.h"

namespace Ethane {

	// The default meshes (res/meshes/default/*.fbx) loaded through assimp, lined up on a textured,
	// normal-mapped floor (assets/models/CobblestoneFloor, an OBJ + MTL) and seen through the EditorCamera
	// (Alt + left drag: orbit, Alt + middle drag: pan, Alt + right drag / scroll: zoom).
	// Shader: assets/shaders/mesh.glsl (set 0: camera, set 1: Ethane::Material).
	class MeshLayer : public Layer
	{
	public:
		MeshLayer() : Layer("MeshLayer") {}

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnEvent(Event& e) override;
		virtual void OnRender(ICommandList& commandList) override;

	private:
		struct Model
		{
			Ref<Mesh> MeshData;
			glm::mat4 Normalize{ 1.0f }; // fits the mesh in a unit box resting on y = 0, centered on x/z
			glm::vec3 Position{ 0.0f };
			std::vector<Ref<Material>> Materials; // by submesh material index
			bool Spin = true;
		};

		EditorCamera m_Camera{ 45.0f, 1.778f, 0.1f, 100.0f };

		std::vector<Model> m_Models;
		RefCountPtr<IShader> m_Shader;
		RefCountPtr<IPipeline> m_Pipeline;
		RefCountPtr<IUniformBuffer> m_CameraBuffer;
		RefCountPtr<IBindingSet> m_BindingSet;

		float m_Time = 0.0f;
	};

}
