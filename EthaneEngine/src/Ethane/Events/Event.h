#pragma once

#include "../Core.h"
#include "Ethane/Core.h"

#include <string>
#include <functional>

namespace Ethane {

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, 
		Apptick, AppUpdate, AppRender, 
		KeyPressed, KeyReleased, 
		MouseButtonPressed, MouseButtonRelease, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication     = BIT(0), 
		EventCategoryInput           = BIT(1),
		EventCategoryKeyboard        = BIT(2),
		EventCategoryMouse           = BIT(3),
		EventCategoryMouseButton     = BIT(4)
	};

#define EVENT_CLASS_TYPE(type) 

	class ETHANE_API Event
	{
		friend class EventDispatcher;
	public:
		//For Debug
		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		inline IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}

	protected:
		bool m_Handled = false;
	};

}