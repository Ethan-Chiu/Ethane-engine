#pragma once

#include "Ethane.h"

#include "Ethane/ImGui/ImGuiLayer.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Ethane/Renderer/EditorCamera.h"

// TODO: test
#include "Ethane/UI/UIImage.h"
#include "Ethane/NodeGraph/NodeGraph.h"

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
		void OnScenePlay();
		void OnSceneStop();

		void NewScene();
		void OpenScene();
		void OpenScene(const std::filesystem::path& filepath);
		void SaveSceneAs();

		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		// UI Panels
		void UI_Toolbar();

		// Helper function
		std::pair<float, float> GetViewportSpaceMousePosition();
		std::pair<glm::vec3, glm::vec3> CastRay(const EditorCamera& camera, float mx, float my);
	private:
		Ref<Shader> m_FlatColorShader;
		Ref<Framebuffer> m_Framebuffer;

		EditorCamera m_EditorCamera;
		// OrthographicCameraController m_CameraController;

		enum class SceneState
		{
			Edit = 0, Play = 1
		};
		SceneState m_SceneState = SceneState::Edit;

		Ref<Scene> m_ActiveScene;
		Ref<SceneRenderer> m_ViewportRenderer;

		Entity m_HoveredEntity;

		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];
		bool m_ViewportResize = false;

		int m_GizmoType = -1;

		//Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		ContentBrowserPanel m_ContentBrowserPanel;
		// TODO: test
		NodeGraph m_NodeGraph;

		bool m_PrimaryCamera = true;

		// Assets
		Ref<Texture2D> m_IconPlay, m_IconStop;

		// TODO: test remove
		// Ref<Texture2D> m_TexTest = nullptr;

	};

}