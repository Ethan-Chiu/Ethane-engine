#pragma once

#include "Ethane.h"

#include "Ethane/ImGui/ImGuiLayer.h"

#include "SCNodeGraph.h"

namespace Ethane {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:

		// UI Panels
		void UI_Toolbar();
	private:

		// TODO: test
		NodeGraph::SCNG m_NodeGraph;
	};

}