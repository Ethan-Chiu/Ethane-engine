#include "ethpch.h"
#include "Application.h"
#include "Ethane/Core/Log.h"
#include "Input.h"

#include "Ethane/Asset/AssetManager.h"
#include "Ethane/Renderer/Renderer.h"

#include <GLFW/glfw3.h>


namespace Ethane
{

#define BIND_EVENT_FUNCTION(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name, ApplicationCommandLineArgs args)
		: m_CommandLineArgs(args)
	{
		ETH_PROFILE_FUNCTION();

		ETH_CORE_ASSERT(!s_Instance, "Application already exists!")
		s_Instance = this;

		m_Window = Window::Create(WindowProps(name));
		m_Window->SetEventCallback(BIND_EVENT_FUNCTION(OnEvent));
		// m_Window->SetVSync(false); // deal with this tomorrow

		// AssetManager::Init();
		// Renderer::Init();

		// m_ImGuiLayer = new ImGuiLayer;
		// PushOverlay(m_ImGuiLayer);
	}
	Application::~Application()
	{
		ETH_PROFILE_FUNCTION();

		Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		ETH_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		ETH_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		ETH_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FUNCTION(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FUNCTION(OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		ETH_PROFILE_FUNCTION();

		while (m_Running)
		{
			ETH_PROFILE_SCOPE("RunLoop");

			float time = glfwGetTime(); //plateform GetTime()
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (false && !m_Minimized)
			{
				Renderer::BeginFrame();

				{
					ETH_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}

				m_ImGuiLayer->Begin();
				{
					ETH_PROFILE_SCOPE("LayerStack OnImGuiRender");

					for (Layer* layer : m_LayerStack)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();

				Renderer::EndFrame();
			}

			// auto [x, y] = Input::GetMousePosition();
			// ETH_CORE_TRACE("{0}, {1}", x, y);

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		ETH_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		// Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
		m_Window->OnResize(e.GetWidth(), e.GetHeight());
		return false;
	}
}