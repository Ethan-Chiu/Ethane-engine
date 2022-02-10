#include "ethpch.h"
#include "Window.h"

#ifdef ETH_PLATFORM_WINDOWS
	#include "Ethane/Platform/Windows/WindowsWindow.h"
#endif

namespace Ethane
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
	#ifdef ETH_PLATFORM_WINDOWS
			return CreateScope<WindowsWindow>(props);
	#else
			ETH_CORE_ASSERT(false, "Unknown platform!");
			return nullptr;
	#endif
	}

}