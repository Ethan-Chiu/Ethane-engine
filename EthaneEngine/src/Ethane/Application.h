#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Ethane/Events/ApplicationEvent.h"
#include "Window.h"
#include "Ethane/LayerStack.h"
#include "Ethane/Core/Timestep.h"
#include "Ethane/ImGui/ImGuiLayer.h"


namespace Ethane
{
	class Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
		
		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline static Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_Window; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
	private:
		static Application* s_Instance;
	};

	Application* CreateApplication();
		//to be define in client
}