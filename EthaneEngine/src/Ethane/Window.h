#pragma once

#include "ethpch.h"
#include "Ethane/Core.h"
#include "Ethane/Events/Event.h"

namespace Ethane {

	struct WindowProps 
	{
		std::string Title;
		unsigned int Width;
		unsigned int Height;

		WindowPorps(const std::string& title = "Ethane Engine,
					unsigned int width = 1280
					unsigned int height = 720)
			:Title(title), Width(width), Height(height) {}
	};

	class ETHANE_API Window
	{
	public:
		using EventCallBackFn = std::function<void(Event&)>;

		virtual ~Window() {}

		virtual void OnUpdate() = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		virtual void SetEventCallBack(const EventCallBackFn& callback) = 0;
		virtual void SetVSync(bool enable) = 0;
		virtual bool IsSync() const = 0;

		static Window* Create(const WindowProps& props = WindowProps());
	};

}