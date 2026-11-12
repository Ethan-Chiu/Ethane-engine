#pragma once

#include "Ethane.h"
#include "Ethane/Asset/AssetLibrary.h"
#include "Ethane/Asset/AssetRegistry.h"
#include "Ethane/Renderer/EditorCamera.h"
#include "Ethane/Renderer/FramePacket.h"
#include "Ethane/Renderer/Renderer.h"

#include "EditorContext.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"

#include <glm/glm.hpp>

namespace Ethane {

	// The editor UI: a dockspace with a menu bar, the active stele's edit world rendered into a Viewport
	// panel (EditorCamera: Alt + left drag orbit, Alt + middle drag pan, Alt + right drag / scroll zoom,
	// while the cursor is over the viewport), the Hierarchy and Inspector panels, and Stats.
	//
	// Steles open in an EditorContext; panels read their snapshots and change them only through the
	// context's transactions, so every edit can be undone (Cmd/Ctrl+Z, Shift+Cmd/Ctrl+Z).
	//
	// Play (Ctrl+P) builds a play world from the active stele's current snapshot: it runs the scripts and
	// is seen through the stele's primary camera. Stop throws it away; the stele was never changed by it.
	// Edits made while playing go to the stele as usual (they reach the play world too, and are kept).
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
		void DrawStatsPanel();
		void DrawFileDialogs();
		void HandleShortcuts();

		void Play();
		void Stop();
		bool IsPlaying() const { return m_PlayWorld != nullptr; }
		// The editor camera takes input only while it's what the viewport shows (not through a play camera)
		bool EditorCameraInControl() const { return !IsPlaying() || !m_PlayHasCamera; }
		void DrawViewportToolbar();

		void NewStele();
		void NewDemoStele();
		void Save(bool chooseFile);
		void ResetCamera();

	private:
		AssetRegistry m_AssetRegistry;
		Scope<AssetLibrary> m_Assets;
		Scope<Renderer> m_Renderer;
		Scope<EditorContext> m_Context;
		FramePacket m_Packet; // rebuilt every frame (keeps its capacity)

		// Play mode: the world built from the stele when Play was pressed
		Scope<EnttWorld> m_PlayWorld;
		SteleSubscription m_PlayFollow;
		const OpenStele* m_Playing = nullptr; // the stele it was built from
		bool m_PlayHasCamera = false;

		HierarchyPanel m_Hierarchy;
		InspectorPanel m_Inspector;

		EditorCamera m_Camera{ 45.0f, 1.778f, 0.1f, 100.0f };
		RefCountPtr<IRenderTarget> m_ViewportTarget;
		glm::uvec2 m_ViewportSize{ 0 }; // in pixels, as the panel last laid out; applied in OnUpdate
		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		// File > Open / Save as: a path typed or picked in a popup
		bool m_OpenRequested = false;
		bool m_SaveAsRequested = false;
		std::string m_DialogPath;
		std::string m_DialogError;

		bool m_ShowHierarchy = true;
		bool m_ShowInspector = true;
		bool m_ShowStats = true;
		bool m_ShowImGuiDemo = false;
		bool m_ResetLayout = false;
	};

}
