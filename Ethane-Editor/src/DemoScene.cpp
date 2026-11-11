#include "DemoScene.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Ethane {

	namespace {
		// layout(set = 0, binding = 0) uniform Camera { mat4 viewproj; vec4 position; }
		struct CameraData
		{
			glm::mat4 ViewProj;
			glm::vec4 Position;
		};

		// Scales the mesh's bounding box to fit a `size` cube, resting on y = 0, centered on x / z
		glm::mat4 FitToBox(const AABB& bounds, float size)
		{
			const glm::vec3 extent = bounds.Max - bounds.Min;
			const float largest = std::max({ extent.x, extent.y, extent.z, 1e-6f });
			const glm::vec3 anchor{ (bounds.Min.x + bounds.Max.x) * 0.5f, bounds.Min.y, (bounds.Min.z + bounds.Max.z) * 0.5f };
			return glm::scale(glm::mat4(1.0f), glm::vec3(size / largest)) * glm::translate(glm::mat4(1.0f), -anchor);
		}
	}

	DemoScene::DemoScene(GfxBackendAPI& backend, const RefCountPtr<IRenderPass>& renderPass)
	{
		m_Shader = backend.CreateShader("assets/shaders/mesh.glsl");

		PipelineSpecification spec;
		spec.Shader = m_Shader;
		spec.RenderPass = renderPass;
		spec.Layout = Mesh::GetVertexLayout();
		spec.CullMode = CullMode::BACK;
		m_Pipeline = backend.CreatePipeline(spec);

		// The fbx files' own materials, recolored; the cube's base color is a texture instead
		struct Entry { const char* Name; glm::vec4 Color; float Metallic; float Roughness; const char* Texture; };
		const Entry entries[] = {
			{ "Cube",     { 1.00f, 1.00f, 1.00f, 1.0f }, 0.0f, 0.6f, "assets/textures/test.png" },
			{ "Sphere",   { 0.95f, 0.70f, 0.25f, 1.0f }, 1.0f, 0.3f, nullptr }, // gold-ish metal
			{ "Cylinder", { 0.45f, 0.75f, 0.35f, 1.0f }, 0.0f, 0.9f, nullptr },
			{ "Cone",     { 0.30f, 0.65f, 0.80f, 1.0f }, 0.0f, 0.2f, nullptr },
			{ "Torus",    { 0.55f, 0.45f, 0.85f, 1.0f }, 1.0f, 0.5f, nullptr },
			{ "Capsule",  { 0.85f, 0.45f, 0.70f, 1.0f }, 0.0f, 0.4f, nullptr },
		};
		const float spacing = 1.6f;
		const float firstX = -spacing * 0.5f * static_cast<float>(std::size(entries) - 1);
		for (size_t i = 0; i < std::size(entries); i++)
		{
			const Entry& entry = entries[i];
			Ref<Mesh> mesh = Mesh::Load(backend, std::string("res/meshes/default/") + entry.Name + ".fbx");
			if (!mesh) {
				continue; // logged by Mesh::Load
			}
			std::vector<MaterialData> materials = mesh->GetMaterials();
			for (MaterialData& material : materials) {
				material.Name = entry.Name;
				material.BaseColor = entry.Color;
				material.Metallic = entry.Metallic;
				material.Roughness = entry.Roughness;
				if (entry.Texture) {
					material.BaseColorTexture = backend.LoadTexture2D(entry.Texture);
				}
			}
			m_Models.push_back(Model{
				.Name      = entry.Name,
				.MeshData  = mesh,
				.Normalize = FitToBox(mesh->GetBoundingBox(), 1.0f),
				.Position  = { firstX + spacing * static_cast<float>(i), 0.0f, 0.0f },
				.Materials = Material::CreateAll(backend, m_Shader, materials),
				.Spin      = true});
		}

		// Textures and factors straight from the file's .mtl
		if (Ref<Mesh> floor = Mesh::Load(backend, "assets/models/CobblestoneFloor/CobblestoneFloor.obj")) {
			const AABB& bounds = floor->GetBoundingBox();
			const float size = std::max(bounds.Max.x - bounds.Min.x, bounds.Max.z - bounds.Min.z);
			const glm::mat4 fit = glm::scale(glm::mat4(1.0f), glm::vec3(12.0f / std::max(size, 1e-6f)))
			                    * glm::translate(glm::mat4(1.0f), -0.5f * (bounds.Min + bounds.Max));
			m_Models.push_back(Model{
				.Name      = "Floor",
				.MeshData  = floor,
				.Normalize = fit,
				.Materials = Material::CreateAll(backend, m_Shader, floor->GetMaterials()),
				.Spin      = false});
		}

		m_CameraBuffer = backend.CreateUniformBuffer(sizeof(CameraData));
		m_BindingSet = backend.CreateBindingSet(m_Shader, 0);
		m_BindingSet->SetUniformBuffer(0, m_CameraBuffer);
	}

	void DemoScene::OnUpdate(Timestep ts)
	{
		m_Time += ts * SpinSpeed;
	}

	void DemoScene::Render(ICommandList& commandList, const EditorCamera& camera)
	{
		const CameraData cameraData{ camera.GetViewProjection(), glm::vec4(camera.GetPosition(), 1.0f) };
		m_CameraBuffer->SetData(&cameraData, sizeof(cameraData));

		commandList.BindPipeline(*m_Pipeline);
		commandList.BindBindingSet(*m_BindingSet);

		for (const Model& model : m_Models)
		{
			if (!model.Visible) {
				continue;
			}
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), model.Position);
			if (model.Spin) {
				transform = glm::rotate(transform, m_Time, glm::vec3(0.0f, 1.0f, 0.0f));
			}
			transform = glm::scale(transform, glm::vec3(model.Scale));
			model.MeshData->Draw(commandList, transform * model.Normalize, model.Materials);
		}
	}

}
