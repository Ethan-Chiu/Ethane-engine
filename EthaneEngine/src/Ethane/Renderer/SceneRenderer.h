#pragma once

#include "Renderer.h"
#include "Material.h"

#include "Ethane/Scene/Scene.h"

// TODO temp move to renderer?
#include "Ethane/Renderer/RenderCommandBuffer.h"

namespace Ethane {

	class Scene;

	struct SceneRendererOptions
	{
		bool ShowGrid = true;
	};

	class SceneRenderer
	{
	public:
		SceneRenderer(Ref<Scene> scene);

		void Init();

		void SetViewportSize(uint32_t width, uint32_t height);

		void BeginScene(const EditorCamera& camera);
		void EndScene();

		void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> material = nullptr);
		void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> Material = nullptr);

		// Getter
		SceneRendererOptions& GetOptions() { return m_Options; }
		Ref<Image2D> GetFinalPassImage();

	private:
		void Flush();

		void GeometryPass();
		
		void CompositePass();

		

	private:
		SceneRendererOptions m_Options;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		float m_InvViewportWidth = 0.f, m_InvViewportHeight = 0.f;
		bool m_NeedResize = false;

		Ref<Scene> m_Scene;

		Ref<RenderCommandBuffer> m_CommandBuffer;

		struct UBCamera
		{
			glm::mat4 ViewProjection;
			glm::mat4 InverseViewProjection;
			glm::mat4 Projection;
			glm::mat4 View;
		} CameraDataUB;

		struct UBScreenData
		{
			glm::vec2 InvFullResolution;
			glm::vec2 FullResolution;
		} ScreenDataUB;

		struct UBRendererData
		{
			glm::vec4 CascadeSplits;
			uint32_t TilesCountX{ 0 };
			bool ShowCascades = false;
			char Padding0[3] = { 0,0,0 }; // Bools are 4-bytes in GLSL
			bool SoftShadows = true;
			char Padding1[3] = { 0,0,0 };
			float LightSize = 0.5f;
			float MaxShadowDistance = 200.0f;
			float ShadowFade = 1.0f;
			bool CascadeFading = true;
			char Padding2[3] = { 0,0,0 };
			float CascadeTransitionFade = 1.0f;
			bool ShowLightComplexity = false;
			char Padding3[3] = { 0,0,0 };
		} RendererDataUB;

		Ref<Material> m_CompositeMaterial;
		
		Ref<Pipeline> m_GeometryPipeline;
		Ref<Pipeline> m_SelectedGeometryPipeline;
		Ref<Pipeline> m_GeometryWireframePipeline;
		Ref<Pipeline> m_GeometryWireframeOnTopPipeline;
		Ref<Pipeline> m_PreDepthPipeline;
		Ref<Pipeline> m_CompositePipeline;
		Ref<Pipeline> m_ShadowPassPipelines[4];
		Ref<Material> m_ShadowPassMaterial;
		Ref<Material> m_PreDepthMaterial;
		Ref<Pipeline> m_SkyboxPipeline;
		Ref<Material> m_SkyboxMaterial;

		// Grid
		Ref<Pipeline> m_GridPipeline;
		Ref<Shader> m_GridShader;

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			glm::mat4 Transform;
			Ref<Material> Material;
		};
		std::vector<DrawCommand> m_DrawList;
		std::vector<DrawCommand> m_SelectedMeshDrawList;

		// TODO: remove
		Ref<Texture2D> m_Texture2D = nullptr;
		Ref<Mesh> m_testMesh = nullptr;
		Ref<Material> m_testMaterial = nullptr;
	};

}