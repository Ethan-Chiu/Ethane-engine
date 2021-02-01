#pragma once

#include "Ethane/Core/Core.h"
#include "Ethane/Events/Event.h"
#include "Ethane/Core/Timestep.h"

namespace Ethane {

	class ETHANE_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};
}