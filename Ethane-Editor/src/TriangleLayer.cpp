#include "TriangleLayer.h"
#include "Ethane/Renderer/RenderWork.h"

namespace Ethane {

	namespace {
		// Matches the vertex input of assets/shaders/triangle.glsl
		struct Vertex
		{
			float Position[2];
			float Color[3];
		};

		// Clip space, +Y up, left half of the window; counter-clockwise, so it survives back-face culling
		constexpr Vertex kVertices[] = {
			{ { -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f } }, // top
			{ { -0.9f, -0.5f }, { 0.0f, 0.0f, 1.0f } }, // bottom left
			{ { -0.1f, -0.5f }, { 0.0f, 1.0f, 0.0f } }, // bottom right
		};
		constexpr uint32_t kIndices[] = { 0, 1, 2 };
	}

	void TriangleLayer::OnAttach()
	{
		Application& app = Application::Get();
		GfxBackendAPI& backend = app.GetGfxBackend();

		m_VertexBuffer = backend.CreateVertexBuffer(sizeof(kVertices));
		m_VertexBuffer->SetData(kVertices, sizeof(kVertices));

		m_IndexBuffer = backend.CreateIndexBuffer(sizeof(kIndices));
		m_IndexBuffer->SetData(kIndices, sizeof(kIndices));

		PipelineSpecification spec;
		spec.Shader = backend.CreateShader("assets/shaders/triangle.glsl");
		spec.RenderPass = app.GetWindowTarget().GetRenderPass(); // draws straight into the window
		spec.Layout = {
			{ ShaderDataType::Float2, "a_Position" },
			{ ShaderDataType::Float3, "a_Color" },
		};
		spec.CullMode = CullMode::BACK;
		m_Pipeline = backend.CreatePipeline(spec);
	}

	void TriangleLayer::OnDetach()
	{
		// GPU resources must go before the backend shuts down (layers are detached first)
		m_Pipeline.reset();
		m_IndexBuffer.reset();
		m_VertexBuffer.reset();
	}

	void TriangleLayer::OnSubmitRender(RenderWork& work)
	{
		work.Window([pipeline = m_Pipeline, vertices = m_VertexBuffer, indices = m_IndexBuffer](ICommandList& commandList) {
			commandList.BindPipeline(*pipeline);
			commandList.BindVertexBuffer(*vertices);
			commandList.BindIndexBuffer(*indices);
			commandList.DrawIndexed(indices->GetCount());
		});
	}

}
