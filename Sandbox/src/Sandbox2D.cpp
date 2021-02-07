#include "Sandbox2D.h"
#include "imgui/imgui.h"

#include "Platform/OpenGL/OpenGLShader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static const uint32_t s_MapWidth = 24;
static const char* s_MapTiles =
"WWWWWWWWWWWWWWWWWWWWWWWW"
"WWWWDDWWWWWWWWWWWWWWWWWW"
"WWDDDDDWWWWWWWWWWWWWWWWW"
"WWWDDWWWWWWWWWWWWWWWWWWW"
"WWWWWWWWWWWWWWWWWWWWWWWW"
"WWWWWWWDDDDDDDDWWWWWWWWW"
"WWWWWWDDDDDDDDDDDDDWWWWW"
"WWWWWDDDDDDDDDDWWWWWWWWW"
"WWWWDDDDDDDDDWWWWWWWWWWW"
"WWWDDDDDDDWWWWWWWDDWWWWW"
"WWWWDDDDWWWWWWWDDDDWWWWW"
"WWWWWWWWWWWWWWDDDDDDDWWW"
"WWWWWWWWWWWWWWWWDDDDWWWW"
"WWWWWWWWWWWWWWWWWWWWWWWW"
;

Sandbox2D::Sandbox2D()
	:Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f)
{
}

void Sandbox2D::OnAttach()
{
	ETH_PROFILE_FUNCTION();

	m_Texture = Ethane::Texture2D::Create("assets/textures/test.png");
	m_SpriteSheet = Ethane::Texture2D::Create("assets/textures/RPGpack_sheet_2X.png");

	m_TextureStairs = Ethane::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 6 }, { 128, 128 });
	m_TextureBarrel = Ethane::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 8, 2 }, { 128, 128 });
	m_TextureTree = Ethane::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 2, 1 }, { 128, 128 }, {1, 2});

	s_TextureMap['D'] = Ethane::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 6, 11 }, { 128, 128 });
	s_TextureMap['W'] = Ethane::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 11, 11 }, { 128, 128 });
	m_MapWidth = s_MapWidth;
	m_MapHeight = strlen(s_MapTiles) / s_MapWidth;
	// m_CameraController.SetZoomLevel(0.5f);
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
	Ethane::Renderer2D::ResetStats();
	{
		ETH_PROFILE_SCOPE("Renderer Prep");
		Ethane::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		Ethane::RenderCommand::Clear();
	}

	{
		ETH_PROFILE_SCOPE("Renderer Draw");
		/*static float rotation = 0.0f;
		rotation += 10 * ts;

		Ethane::Renderer2D::BeginScene(m_CameraController.GetCamera());

		Ethane::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
		Ethane::Renderer2D::DrawRotatedQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, -45.0f, { 0.8f, 0.2f, 0.3f, 1.0f });
		Ethane::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
		Ethane::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 6.0f, 6.0f }, m_Texture, 3.0f);
		Ethane::Renderer2D::DrawRotatedQuad({ -0.0f, -0.0f, -0.0f }, { 1.0f, 1.0f }, rotation, m_Texture, 20.f);
		// Ethane::Renderer2D::DrawRotatedQuad({ 0.0f, 0.0f, -0.1f }, { 3.0f, 3.0f }, 45.0f, m_Texture);

		Ethane::Renderer2D::EndScene();

		Ethane::Renderer2D::BeginScene(m_CameraController.GetCamera());
		for (float y = -5.0f; y <= 5.0f; y += 0.5f)
		{
			for (float x = -5.0f; x <= 5.0f; x += 0.5f)
			{
				glm::vec4 color = { (x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.5f };
				Ethane::Renderer2D::DrawQuad({ x, y }, { 0.45f, 0.45f }, color);
			}
		}
		Ethane::Renderer2D::EndScene();*/
		Ethane::Renderer2D::BeginScene(m_CameraController.GetCamera());
		// Ethane::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, m_TextureStairs);
		// Ethane::Renderer2D::DrawQuad({ 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, m_TextureBarrel); 
		// Ethane::Renderer2D::DrawQuad({ 1.0f, 0.0f, 0.0f }, { 1.0f, 2.0f }, m_TextureTree);
		for (uint32_t y = 0; y < m_MapHeight; y++)
		{
			for (uint32_t x = 0; x < m_MapWidth; x++)
			{
				char tileType = s_MapTiles[x + y * m_MapWidth];
				Ethane::Ref<Ethane::SubTexture2D> texture;
				if (s_TextureMap.find(tileType) != s_TextureMap.end())
					texture = s_TextureMap[tileType];
				else
					texture = m_TextureBarrel;
				Ethane::Renderer2D::DrawQuad({ x - m_MapWidth / 2.0f, m_MapHeight / 2.0f - y, 0.5f }, { 1.0f, 1.0f }, texture);
			}
		}
		Ethane::Renderer2D::EndScene();
	}
}

void Sandbox2D::OnImGuiRender()
{
	ETH_PROFILE_FUNCTION();

	ImGui::Begin("Settings");

	auto stats = Ethane::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());


	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));

	ImGui::End();
}

void Sandbox2D::OnEvent(Ethane::Event& e)
{
	m_CameraController.OnEvent(e);
}