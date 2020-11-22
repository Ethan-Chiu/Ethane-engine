#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Ethane/Events/ApplicationEvent.h"
#include "Window.h"
#include "Ethane/LayerStack.h"

#include "Ethane/ImGui/ImGuiLayer.h"

#include "Ethane/Renderer/Shader.h"
#include "Ethane/Renderer/Buffer.h"

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

		inline static Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_Window; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		LayerStack m_LayerStack;

		unsigned int m_VertexArray;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
	private:
		static Application* s_Instance;
	};

	Application* CreateApplication();
		//to be define in client
}