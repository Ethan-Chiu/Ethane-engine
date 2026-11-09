#include "TexturedQuadLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Ethane {

	namespace {
		// Matches the vertex input of assets/shaders/test.glsl
		struct Vertex
		{
			float Position[3];
			float Color[3];
			float TexCoord[2];
		};

		// Counter-clockwise, +Y up. v = 0 is the image's top row, so the top vertices get v = 0.
		constexpr Vertex kVertices[] = {
			{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }, // bottom left
			{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }, // bottom right
			{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }, // top right
			{ { -0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }, // top left
		};
		constexpr uint32_t kIndices[] = { 0, 1, 2, 2, 3, 0 };

		// layout(binding = 0) uniform UniformBufferObject { mat4 viewproj; }
		struct CameraData
		{
			glm::mat4 ViewProj;
		};
	}

	void TexturedQuadLayer::OnAttach()
	{
		Application& app = Application::Get();
		GfxBackendAPI& backend = app.GetGfxBackend();

		m_VertexBuffer = backend.CreateVertexBuffer(sizeof(kVertices));
		m_VertexBuffer->SetData(kVertices, sizeof(kVertices));
		m_IndexBuffer = backend.CreateIndexBuffer(sizeof(kIndices));
		m_IndexBuffer->SetData(kIndices, sizeof(kIndices));

		m_Shader = backend.CreateShader("assets/shaders/test.glsl");

		PipelineSpecification spec;
		spec.Shader = m_Shader;
		spec.RenderPass = app.GetWindowTarget().GetRenderPass();
		spec.Layout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
		};
		spec.CullMode = CullMode::BACK;
		m_Pipeline = backend.CreatePipeline(spec);

		m_CameraBuffer = backend.CreateUniformBuffer(sizeof(CameraData));
		m_Texture = backend.LoadTexture2D("assets/textures/test.png");

		m_BindingSet = backend.CreateBindingSet(m_Shader, 0);
		m_BindingSet->SetUniformBuffer(0, m_CameraBuffer);
		m_BindingSet->SetTexture(1, m_Texture);
	}

	void TexturedQuadLayer::OnDetach()
	{
		m_BindingSet.reset();
		m_Texture.reset();
		m_CameraBuffer.reset();
		m_Pipeline.reset();
		m_Shader.reset();
		m_IndexBuffer.reset();
		m_VertexBuffer.reset();
	}

	void TexturedQuadLayer::OnUpdate(Timestep ts)
	{
		m_Time += ts;
	}

	void TexturedQuadLayer::OnRender(ICommandList& commandList)
	{
		const Window& window = Application::Get().GetWindow();
		const float aspect = window.GetHeight() > 0 ? static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()) : 1.0f;

		// Aspect-correct view of [-aspect, aspect] x [-1, 1]; the quad spins in the right half
		const glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
		const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(aspect * 0.5f, 0.0f, 0.0f))
		                      * glm::rotate(glm::mat4(1.0f), m_Time, glm::vec3(0.0f, 0.0f, 1.0f))
		                      * glm::scale(glm::mat4(1.0f), glm::vec3(0.7f));
		const CameraData camera{ projection * model };
		// per-frame data: written during OnRender, into the copy this frame reads
		m_CameraBuffer->SetData(&camera, sizeof(camera));

		commandList.BindPipeline(*m_Pipeline);
		commandList.BindBindingSet(*m_BindingSet);
		commandList.BindVertexBuffer(*m_VertexBuffer);
		commandList.BindIndexBuffer(*m_IndexBuffer);
		commandList.DrawIndexed(m_IndexBuffer->GetCount());
	}

}
