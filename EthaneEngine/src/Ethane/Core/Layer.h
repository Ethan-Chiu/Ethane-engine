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
		// Called after OnUpdate (and the ImGui UI is built), only when a frame is being rendered. First every
		// layer's OnRenderOffscreen, outside any render pass: draw into IRenderTargets here. Then every
		// layer's OnRender, inside the window's render pass.
		virtual void OnRenderOffscreen(ICommandList& commandList) {}
		virtual void OnRender(ICommandList& commandList) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};
}