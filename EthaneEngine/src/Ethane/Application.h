#pragma once

#include "Core.h"
#include "Events\Event.h"

namespace Ethane
{
	class ETHANE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
	};

	Application* CreateApplication();
		//to be define in client
}