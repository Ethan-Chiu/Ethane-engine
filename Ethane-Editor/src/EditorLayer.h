#pragma once

#include "Ethane.h"

#include "Ethane/ImGui/ImGuiLayer.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Ethane/Renderer/EditorCamera.h"

// TODO: test
#include "Ethane/UI/UIImage.h"

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
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void NewScene();
		void OpenScene();
		void SaveSceneAs();
	private:
		Ref<Shader> m_FlatColorShader;
		Ref<Framebuffer> m_Framebuffer;

		EditorCamera m_EditorCamera;
		// OrthographicCameraController m_CameraController;

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

		bool m_PrimaryCamera = true;

		// TODO: test remove
		// Ref<Texture2D> m_TexTest = nullptr;


		// Ref<Texture2D> m_Texture;
		// Ref<Texture2D> m_SpriteSheet;
		// Ref<SubTexture2D> m_TextureStairs, m_TextureBarrel, m_TextureTree;
	};

}