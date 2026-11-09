#include "CubeLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Ethane {

	namespace {
		// Matches the vertex input of assets/shaders/cube.glsl
		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec2 TexCoord;
		};

		// layout(set = 0, binding = 0) uniform Camera { mat4 viewproj; }
		struct CameraData
		{
			glm::mat4 ViewProj;
		};

		// Unit cube, 4 vertices per face so each face gets its own normal and texture coordinates
		void BuildCube(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
		{
			struct Face { glm::vec3 Normal, U, V; }; // U x V = Normal: corners in U/V order are counter-clockwise from outside
			const Face faces[] = {
				{ {  1,  0,  0 }, {  0,  0, -1 }, { 0, 1,  0 } },
				{ { -1,  0,  0 }, {  0,  0,  1 }, { 0, 1,  0 } },
				{ {  0,  1,  0 }, {  1,  0,  0 }, { 0, 0, -1 } },
				{ {  0, -1,  0 }, {  1,  0,  0 }, { 0, 0,  1 } },
				{ {  0,  0,  1 }, {  1,  0,  0 }, { 0, 1,  0 } },
				{ {  0,  0, -1 }, { -1,  0,  0 }, { 0, 1,  0 } },
			};
			for (const Face& face : faces) {
				const uint32_t base = static_cast<uint32_t>(vertices.size());
				const glm::vec3 center = face.Normal * 0.5f;
				// v = 0 is the image's top row
				vertices.push_back({ center - face.U * 0.5f - face.V * 0.5f, face.Normal, { 0.0f, 1.0f } });
				vertices.push_back({ center + face.U * 0.5f - face.V * 0.5f, face.Normal, { 1.0f, 1.0f } });
				vertices.push_back({ center + face.U * 0.5f + face.V * 0.5f, face.Normal, { 1.0f, 0.0f } });
				vertices.push_back({ center - face.U * 0.5f + face.V * 0.5f, face.Normal, { 0.0f, 0.0f } });
				for (uint32_t i : { 0u, 1u, 2u, 2u, 3u, 0u }) {
					indices.push_back(base + i);
				}
			}
		}
	}

	void CubeLayer::OnAttach()
	{
		Application& app = Application::Get();
		GfxBackendAPI& backend = app.GetGfxBackend();

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		BuildCube(vertices, indices);
		const uint32_t vertexBytes = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
		const uint32_t indexBytes = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
		m_VertexBuffer = backend.CreateVertexBuffer(vertexBytes);
		m_VertexBuffer->SetData(vertices.data(), vertexBytes);
		m_IndexBuffer = backend.CreateIndexBuffer(indexBytes);
		m_IndexBuffer->SetData(indices.data(), indexBytes);

		m_Shader = backend.CreateShader("assets/shaders/cube.glsl");

		PipelineSpecification spec;
		spec.Shader = m_Shader;
		spec.RenderPass = app.GetWindowTarget().GetRenderPass();
		spec.Layout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float2, "a_TexCoord" },
		};
		spec.CullMode = CullMode::BACK;
		m_Pipeline = backend.CreatePipeline(spec);

		m_CameraBuffer = backend.CreateUniformBuffer(sizeof(CameraData));
		m_Texture = backend.LoadTexture2D("assets/textures/test.png");

		m_BindingSet = backend.CreateBindingSet(m_Shader, 0);
		m_BindingSet->SetUniformBuffer(0, m_CameraBuffer);
		m_BindingSet->SetTexture(1, m_Texture);

		m_Camera.SetDistance(4.0f);
	}

	void CubeLayer::OnDetach()
	{
		m_BindingSet.reset();
		m_Texture.reset();
		m_CameraBuffer.reset();
		m_Pipeline.reset();
		m_Shader.reset();
		m_IndexBuffer.reset();
		m_VertexBuffer.reset();
	}

	void CubeLayer::OnUpdate(Timestep ts)
	{
		m_Time += ts;

		const Window& window = Application::Get().GetWindow();
		m_Camera.SetViewportSize(static_cast<float>(window.GetWidth()), static_cast<float>(window.GetHeight())); // ignores 0 sizes
		m_Camera.OnUpdate(ts);
	}

	void CubeLayer::OnEvent(Event& e)
	{
		m_Camera.OnEvent(e);
	}

	void CubeLayer::OnRender(ICommandList& commandList)
	{
		// per-frame data: written during OnRender, into the copy this frame reads
		const CameraData camera{ m_Camera.GetViewProjection() };
		m_CameraBuffer->SetData(&camera, sizeof(camera));

		commandList.BindPipeline(*m_Pipeline);
		commandList.BindBindingSet(*m_BindingSet);
		commandList.BindVertexBuffer(*m_VertexBuffer);
		commandList.BindIndexBuffer(*m_IndexBuffer);

		// The two cubes intersect: the depth buffer decides which faces show where they cross
		const glm::mat4 models[] = {
			glm::rotate(glm::mat4(1.0f), m_Time * 0.6f, glm::normalize(glm::vec3(0.3f, 1.0f, 0.2f))),
			glm::translate(glm::mat4(1.0f), glm::vec3(0.55f, 0.25f, 0.35f))
				* glm::rotate(glm::mat4(1.0f), -m_Time, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::scale(glm::mat4(1.0f), glm::vec3(0.6f)),
		};
		for (const glm::mat4& model : models) {
			commandList.PushConstants(&model, sizeof(model));
			commandList.DrawIndexed(m_IndexBuffer->GetCount());
		}
	}

}
