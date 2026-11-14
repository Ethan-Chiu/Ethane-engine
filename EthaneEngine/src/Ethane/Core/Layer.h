#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/Events/Event.h"
#include "Ethane/Core/Timestep.h"

namespace Ethane {

	class RenderWork;

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		// Called every frame after OnUpdate and the UI: hand this frame's GPU work over as passes that own
		// what they draw (see RenderWork). Recording happens afterwards, possibly elsewhere: passes must not
		// read the layer's state.
		virtual void OnSubmitRender(RenderWork& work) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};
}