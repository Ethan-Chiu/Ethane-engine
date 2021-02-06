#include "Sandbox2D.h"
#include "imgui/imgui.h"

#include "Platform/OpenGL/OpenGLShader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D()
	:Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f)
{
}

void Sandbox2D::OnAttach()
{
	ETH_PROFILE_FUNCTION();

	m_Texture = Ethane::Texture2D::Create("assets/textures/test.png");
}

void Sandbox2D::OnDetach()
{
	ETH_PROFILE_FUNCTION();

}

void Sandbox2D::OnUpdate(Ethane::Timestep ts)
{
	ETH_PROFILE_FUNCTION();

	//Update
	m_CameraController.OnUpdate(ts);

	//Render
	{
		ETH_PROFILE_SCOPE("Renderer Prep");
		Ethane::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		Ethane::RenderCommand::Clear();
	}

	{
		ETH_PROFILE_SCOPE("Renderer Draw");
		
		Ethane::Renderer2D::BeginScene(m_CameraController.GetCamera());

		Ethane::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
		// Ethane::Renderer2D::DrawRotatedQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, glm::radians(-45.0f), { 0.8f, 0.2f, 0.3f, 1.0f });
		Ethane::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
		Ethane::Renderer2D::DrawQuad({ -3.0f, -3.0f, -0.1f }, { 6.0f, 6.0f }, m_Texture, 10.f);
		Ethane::Renderer2D::DrawQuad({ -0.5f, -0.5f, -0.0f }, { 1.0f, 1.0f }, m_Texture, 20.f);
		// Ethane::Renderer2D::DrawRotatedQuad({ 0.0f, 0.0f, -0.1f }, { 3.0f, 3.0f }, glm::radians(45.0f), m_Texture);

		Ethane::Renderer2D::EndScene();
	}
}

void Sandbox2D::OnImGuiRender()
{
	ETH_PROFILE_FUNCTION();

	ImGui::Begin("Settings");
	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));

	ImGui::End();
}

void Sandbox2D::OnEvent(Ethane::Event& e)
{
	m_CameraController.OnEvent(e);
}