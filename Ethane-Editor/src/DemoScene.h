#pragma once

#include "Ethane.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Renderer/FramePacket.h"
#include "Ethane/Renderer/Mesh.h"

namespace Ethane {

	// The default meshes (res/meshes/default/*.fbx) lined up on a textured, normal-mapped floor
	// (assets/models/CobblestoneFloor), lit by one directional light. Produces frame packets for the
	// Renderer. A stand-in until steles and worlds carry meshes.
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

		// `materialShader`: the shader its materials are created for (Renderer::GetMeshShader)
		DemoScene(GfxBackendAPI& backend, const RefCountPtr<IShader>& materialShader);

		void OnUpdate(Timestep ts);
		// Adds the visible models and the light (the view is the caller's)
		void Extract(FramePacket& packet) const;

		std::vector<Model>& GetModels() { return m_Models; }
		float SpinSpeed = 0.5f; // radians per second
		DirectionalLight Sun{ .Direction = -glm::normalize(glm::vec3(0.4f, 0.8f, 0.6f)), .Color = glm::vec3(1.0f), .Intensity = 0.9f };

	private:
		std::vector<Model> m_Models;
		float m_Time = 0.0f;
	};

}
