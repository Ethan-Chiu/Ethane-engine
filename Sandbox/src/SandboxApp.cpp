#include <Ethane.h>

class ExampleLayer : public Ethane::Layer
{
public:
	ExampleLayer()
		:Layer("Example")
	{
	}

	void OnUpdate() override
	{
		if (Ethane::Input::IsKeyPressed(ETH_KEY_TAB))
		{
			ETH_TRACE("Tab key is pressed");
		}
		//ETH_INFO("ExampleLayer::Update");
	}

	void OnEvent(Ethane::Event& event) override
	{
		// ETH_TRACE("{0}", event);
	}

};


class Sandbox : public Ethane::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}
	~Sandbox()
	{

	}
};

Ethane::Application* Ethane::CreateApplication()
{
	return new Sandbox();
}