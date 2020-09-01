#include "ethpch.h"
#include "Application.h"

#include "Ethane/Events/ApplicationEvent.h"
#include "Ethane/Log.h"

namespace Ethane
{
    Application::Application()
    {

    }
    Application::~Application()
    {

    }
    void Application::Run()
    {
        WindowResizeEvent e(1280, 720); 
        ETH_TRACE(e);
        while (true);
    }
}