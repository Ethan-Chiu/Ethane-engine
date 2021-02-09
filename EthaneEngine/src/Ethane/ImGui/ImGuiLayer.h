#pragma once

#include "Ethane/Core/Layer.h"
#include "Ethane/Events/MouseEvent.h"
#include "Ethane/Events/KeyEvent.h"
#include "Ethane/Events/ApplicationEvent.h"

namespace Ethane {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }
	private:
		bool m_BlockEvents = true;
		float m_Time = 0.0f;
	};
}