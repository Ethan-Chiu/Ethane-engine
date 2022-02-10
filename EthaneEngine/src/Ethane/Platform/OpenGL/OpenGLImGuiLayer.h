#pragma once

#include "Ethane/ImGui/ImGuiLayer.h"
#include "Ethane/Events/MouseEvent.h"
#include "Ethane/Events/KeyEvent.h"
#include "Ethane/Events/ApplicationEvent.h"

namespace Ethane {

	class OpenGLImGuiLayer : public ImGuiLayer
	{
	public:
		OpenGLImGuiLayer();
		~OpenGLImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		virtual void Begin() override;
		virtual void End() override;

		virtual void BlockEvents(bool block) override { m_BlockEvents = block; }
	private:
		bool m_BlockEvents = true;
		float m_Time = 0.0f;
	};
}