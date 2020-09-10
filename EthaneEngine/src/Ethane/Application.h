#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Ethane/Events/ApplicationEvent.h"
#include "Window.h"
#include "Ethane/LayerStack.h"

namespace Ethane
{
	class ETHANE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
		
		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();
		//to be define in client
}