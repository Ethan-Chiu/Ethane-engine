#pragma once

#include <Ethane.h>

class Sandbox2D : public Ethane::Layer
{
public:
	Sandbox2D();
	virtual ~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Ethane::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Ethane::Event& e) override;
private:
	Ethane::OrthographicCameraController m_CameraController;

	Ethane::Ref<Ethane::VertexArray> m_SquareVA;
	Ethane::Ref<Ethane::Shader> m_FlatColorShader;

	Ethane::Ref<Ethane::Texture2D> m_Texture;

	glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };

	struct ProfileResult
	{
		const char* Name;
		float Time;
	};

	std::vector<ProfileResult>m_ProfileResults;
};
