#include <Ethane.h>
//-----EntryPoint-----
#include <Ethane/Core/EntryPoint.h>

#include "SCGameStateEditorLayer.h"
#include "RayTracingLayer.h"

using namespace Ethane;

class ExampleLayer : public Ethane::Layer
{
private:
    

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
};

class Sandbox : public Ethane::Application
{
public:
	Sandbox()
	{
		PushLayer(new RayTracingLayer());
	}
	~Sandbox()
	{

	}
};

Ethane::Application* Ethane::CreateApplication(ApplicationCommandLineArgs args)
{
	RendererAPI::SetAPI(RendererAPI::API::Vulkan);
	return new Sandbox();
}