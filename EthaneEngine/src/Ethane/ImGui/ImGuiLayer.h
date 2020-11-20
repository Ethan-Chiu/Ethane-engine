#pragma once

#include "Ethane/Layer.h"
#include "Ethane/Events/MouseEvent.h"
#include "Ethane/Events/KeyEvent.h"
#include "Ethane/Events/ApplicationEvent.h"

namespace Ethane {

	class ETHANE_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void Begin();
		void End();
	private:
		float m_Time = 0.0f;
	};
}