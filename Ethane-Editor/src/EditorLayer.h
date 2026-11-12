#pragma once

#include "Ethane.h"
#include "Ethane/Renderer/EditorCamera.h"
#include "Ethane/Renderer/FramePacket.h"
#include "Ethane/Renderer/Renderer.h"

#include "DemoScene.h"

#include <glm/glm.hpp>

namespace Ethane {

	// The editor UI: a dockspace with a menu bar, the scene rendered into a Viewport panel (EditorCamera:
	// Alt + left drag orbit, Alt + middle drag pan, Alt + right drag / scroll zoom, while the cursor is
	// over the viewport), a Scene list, a Properties panel for the selected model and its materials,
	// and Stats.
	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnRenderOffscreen(ICommandList& commandList) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;

	private:
		void DrawMenuBar();
		void DrawDockspace();
		void DrawViewport();
		void DrawScenePanel();
		void DrawPropertiesPanel();
		void DrawStatsPanel();

		void ResetCamera();

	private:
		Scope<Renderer> m_Renderer;
		Scope<DemoScene> m_Scene;
		FramePacket m_Packet; // rebuilt every frame (keeps its capacity)
		EditorCamera m_Camera{ 45.0f, 1.778f, 0.1f, 100.0f };

		RefCountPtr<IRenderTarget> m_ViewportTarget;
		glm::uvec2 m_ViewportSize{ 0 }; // in pixels, as the panel last laid out; applied in OnUpdate
		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		int m_SelectedModel = -1; // index into the scene's models

		bool m_ShowScene = true;
		bool m_ShowProperties = true;
		bool m_ShowStats = true;
		bool m_ShowImGuiDemo = false;
		bool m_ResetLayout = false;
	};

}
