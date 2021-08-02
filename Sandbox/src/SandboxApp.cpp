#include <Ethane.h>
//-----EntryPoint-----
#include <Ethane/Core/EntryPoint.h>

using namespace Ethane;

class ExampleLayer : public Ethane::Layer
{
public:
	ExampleLayer()
		:Layer("Example")
	{

	}

	virtual ~ExampleLayer()
	{

	}

	virtual void OnAttach() override
	{

	}

	virtual void OnDetach() override
	{

	}

	void OnUpdate(Ethane::Timestep ts) override
	{

	}

	virtual void OnImGuiRender() override
	{
	}

	void OnEvent(Ethane::Event& e) override
	{
	}
private:
	Ref<RenderPass> m_RenderPass;
};

class Sandbox : public Ethane::Application
{
public:
	Sandbox()
	{
		// PushLayer(new ExampleLayer());
		PushLayer(new ExampleLayer());
	}
	~Sandbox()
	{

	}
};

Ethane::Application* Ethane::CreateApplication(ApplicationCommandLineArgs args)
{
	return new Sandbox();
}