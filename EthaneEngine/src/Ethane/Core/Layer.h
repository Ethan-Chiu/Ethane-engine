#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/Events/Event.h"
#include "Ethane/Core/Timestep.h"

namespace Ethane {

	class ICommandList;

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		// Called after OnUpdate, only when a frame is being rendered: record draw commands here
		virtual void OnRender(ICommandList& commandList) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};
}