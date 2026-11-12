#pragma once

#include "Ethane.h"
#include "Ethane/Asset/AssetLibrary.h"
#include "Ethane/Asset/AssetRegistry.h"
#include "Ethane/Renderer/EditorCamera.h"
#include "Ethane/Renderer/FramePacket.h"
#include "Ethane/Renderer/Renderer.h"
#include "Ethane/Stele/Stele.h"
#include "Ethane/World/Entt/EnttWorld.h"

#include <glm/glm.hpp>

namespace Ethane {

	// The editor UI: a dockspace with a menu bar, the stele's edit world rendered into a Viewport panel
	// (EditorCamera: Alt + left drag orbit, Alt + middle drag pan, Alt + right drag / scroll zoom, while
	// the cursor is over the viewport), a Scene tree of the stele's elements, a Properties panel for the
	// selected element's components, and Stats.
	//
	// The panels read stele snapshots and change the stele only by committing transactions; the edit
	// world follows the stele's change sets.
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
		void DrawElementNode(const SteleSnapshot& snapshot, ElementId id);
		void Commit(Transaction transaction);

	private:
		AssetRegistry m_AssetRegistry;
		Scope<AssetLibrary> m_Assets;
		Scope<Renderer> m_Renderer;
		Scope<Stele> m_Stele;
		Scope<EnttWorld> m_EditWorld;
		SteleSubscription m_EditWorldFollow;
		FramePacket m_Packet; // rebuilt every frame (keeps its capacity)
		EditorCamera m_Camera{ 45.0f, 1.778f, 0.1f, 100.0f };

		RefCountPtr<IRenderTarget> m_ViewportTarget;
		glm::uvec2 m_ViewportSize{ 0 }; // in pixels, as the panel last laid out; applied in OnUpdate
		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		ElementId m_Selected;

		bool m_ShowScene = true;
		bool m_ShowProperties = true;
		bool m_ShowStats = true;
		bool m_ShowImGuiDemo = false;
		bool m_ResetLayout = false;
	};

}
