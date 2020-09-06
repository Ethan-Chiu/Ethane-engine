#include "ethpch.h"
#include "Application.h"

#include "Ethane/Events/ApplicationEvent.h"
#include <GLFW/glfw3.h>

namespace Ethane
{

#define BIND_EVENT_FUNCTION(x) std::bind(&Application::x, this, std::placeholder::_1)

    Application::Application()
    {
        m_Window = std::unique_ptr<Window>(Window::Create());
        m_Window->SetEventCallback(BIND_EVENT_FUNCTION(OnEvent);
    }
    Application::~Application()
    {
        
    }

    void Application::OnEvent(Event& e)
    {
        ETH_CORE_INFO("{0}", e);
    }

    void Application::Run()
    {
        while (m_Running)
        {
            m_Window->OnUpdate();
        }
    }
}