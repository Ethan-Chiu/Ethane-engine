#include "ethpch.h"
#include "Renderer.h"

#include "Platform/OpenGL/OpenGLShader.h"

namespace Ethane {

	Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();
	
	RenderCommandQueue* Renderer::s_CommandQueue = nullptr;
	
	RendererConfig Renderer::s_Config = RendererConfig{};

	void Renderer::Init()
	{
		ETH_PROFILE_FUNCTION();

		RenderCommand::Init();
		s_CommandQueue = new RenderCommandQueue();

		// Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		// Renderer2D::Shutdown();

		delete Renderer::s_CommandQueue;
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(OrthographicCamera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::BeginFrame()
	{
		RenderCommand::BeginFrame();
	}

	void Renderer::EndFrame()
	{
		RenderCommand::EndFrame();
	}

	void Renderer::RenderMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		auto shader = mesh->m_MeshShader;
		shader->Bind();

		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		RenderCommand::DrawMesh(mesh, transform);
	}

	void Renderer::Submit(const Ref<Shader>& shader, const uint32_t indexCount, const glm::mat4& transform)
	{
		shader->Bind();

		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
	 	std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transform);

		RenderCommand::DrawIndexed(indexCount);
	}

	void Renderer::WaitAndRender()
	{
		ETH_PROFILE_FUNCTION();
		s_CommandQueue->Execute();
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue;
	}
	
}
// void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
// {
// 	shader->Bind();
// 	std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
// 	std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transform);
// 
// 	vertexArray->Bind();
// 	RenderCommand::DrawIndexed(vertexArray);
// }
