#include "MeshLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Ethane {

	namespace {
		// layout(set = 0, binding = 0) uniform Camera { mat4 viewproj; }
		struct CameraData
		{
			glm::mat4 ViewProj;
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

	void MeshLayer::OnAttach()
	{
		Application& app = Application::Get();
		GfxBackendAPI& backend = app.GetGfxBackend();

		struct Entry { const char* Name; glm::vec4 Color; };
		const Entry entries[] = {
			{ "Cube",     { 0.85f, 0.35f, 0.30f, 1.0f } },
			{ "Sphere",   { 0.95f, 0.70f, 0.25f, 1.0f } },
			{ "Cylinder", { 0.45f, 0.75f, 0.35f, 1.0f } },
			{ "Cone",     { 0.30f, 0.65f, 0.80f, 1.0f } },
			{ "Torus",    { 0.55f, 0.45f, 0.85f, 1.0f } },
			{ "Capsule",  { 0.85f, 0.45f, 0.70f, 1.0f } },
		};
		const float spacing = 1.6f;
		const float firstX = -spacing * 0.5f * static_cast<float>(std::size(entries) - 1);
		for (size_t i = 0; i < std::size(entries); i++)
		{
			Ref<Mesh> mesh = Mesh::Load(backend, std::string("res/meshes/default/") + entries[i].Name + ".fbx");
			if (!mesh) {
				continue; // logged by Mesh::Load
			}
			const glm::mat4 fit = FitToBox(mesh->GetBoundingBox(), 1.0f);
			m_Models.push_back(Model{ mesh, fit, { firstX + spacing * static_cast<float>(i), 0.0f, 0.0f }, entries[i].Color, true });
		}
		if (Ref<Mesh> plane = Mesh::Load(backend, "res/meshes/default/Plane.fbx")) {
			const AABB& bounds = plane->GetBoundingBox();
			const float size = std::max(bounds.Max.x - bounds.Min.x, bounds.Max.z - bounds.Min.z);
			const glm::mat4 fit = glm::scale(glm::mat4(1.0f), glm::vec3(12.0f / std::max(size, 1e-6f)))
			                    * glm::translate(glm::mat4(1.0f), -0.5f * (bounds.Min + bounds.Max));
			m_Models.push_back(Model{ plane, fit, { 0.0f, 0.0f, 0.0f }, { 0.6f, 0.6f, 0.6f, 1.0f }, false });
		}

		m_Shader = backend.CreateShader("assets/shaders/mesh.glsl");

		PipelineSpecification spec;
		spec.Shader = m_Shader;
		spec.RenderPass = app.GetWindowTarget().GetRenderPass();
		spec.Layout = Mesh::GetVertexLayout();
		spec.CullMode = CullMode::BACK;
		m_Pipeline = backend.CreatePipeline(spec);

		m_CameraBuffer = backend.CreateUniformBuffer(sizeof(CameraData));
		m_BindingSet = backend.CreateBindingSet(m_Shader, 0);
		m_BindingSet->SetUniformBuffer(0, m_CameraBuffer);

		m_Camera.SetDistance(9.0f);
		m_Camera.SetPitchYaw(0.45f, 0.0f);
	}

	void MeshLayer::OnDetach()
	{
		m_BindingSet.reset();
		m_CameraBuffer.reset();
		m_Pipeline.reset();
		m_Shader.reset();
		m_Models.clear();
	}

	void MeshLayer::OnUpdate(Timestep ts)
	{
		m_Time += ts;

		const Window& window = Application::Get().GetWindow();
		m_Camera.SetViewportSize(static_cast<float>(window.GetWidth()), static_cast<float>(window.GetHeight())); // ignores 0 sizes
		m_Camera.OnUpdate(ts);
	}

	void MeshLayer::OnEvent(Event& e)
	{
		m_Camera.OnEvent(e);
	}

	void MeshLayer::OnRender(ICommandList& commandList)
	{
		const CameraData camera{ m_Camera.GetViewProjection() };
		m_CameraBuffer->SetData(&camera, sizeof(camera));

		commandList.BindPipeline(*m_Pipeline);
		commandList.BindBindingSet(*m_BindingSet);

		for (const Model& model : m_Models)
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), model.Position);
			if (model.Spin) {
				transform = glm::rotate(transform, m_Time * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
			}
			// color at offset 64, after the model matrix Mesh::Draw() pushes
			commandList.PushConstants(&model.Color, sizeof(model.Color), sizeof(glm::mat4));
			model.MeshData->Draw(commandList, transform * model.Normalize);
		}
	}

}
