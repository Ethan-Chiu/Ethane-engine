#pragma once

#include "Event.h"
#include <sstream>

namespace Ethane {

	class ETHANE_API KeyEvent : public Event
	{
	public:
		inline int GetKeyCode() const { return m_KeyCode; }
	//	EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	protected:
		KeyEvent(int keycode)
			:m_KeyCode(keycode) {}
		int m_KeyCode;
	};

	class ETHANE_API KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(int keycode, int repeatCount)
			:KeyEvent(keycode), m_RepeatCount(repeatCount) {}
		
		inline int GetRepeatCount() const 
	};
}