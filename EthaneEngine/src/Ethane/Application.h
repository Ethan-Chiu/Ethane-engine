#pragma once

#include "Core.h"

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