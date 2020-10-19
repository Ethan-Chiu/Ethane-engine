#include <Ethane.h>

class ExampleLayer : public Ethane::Layer
{
public:
	ExampleLayer()
		:Layer("Example"){}

	void OnUpdate() override
	{
		ETH_INFO("ExampleLayer::Update");
	}

	void OnEvent(Ethane::Event& event) override
	{
		ETH_TRACE("{0}", event);
	}

};


class Sandbox : public Ethane::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		PushOverlay(new Ethane::ImGuiLayer());
	}
	~Sandbox()
	{

	}
};

Ethane::Application* Ethane::CreateApplication()
{
	return new Sandbox();
}