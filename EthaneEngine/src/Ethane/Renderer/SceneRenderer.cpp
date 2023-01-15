#include "ethpch.h"
#include "SceneRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

// TODO: temp
#include "Vulkan/VulkanRendererAPI.h"
#include "Vulkan/VulkanFramebuffer.h"
// TODO: remove
#include "Vulkan/VulkanMaterial.h"

namespace Ethane {

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_Scene(scene)
	{
		Init();
	}


	void SceneRenderer::Init()
	{
#if 1
		m_CommandBuffer = RenderCommandBuffer::Create(0, "SceneRenderer");

		// Geometry
		{
			FramebufferSpecification geoFramebufferSpec;
			geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA16F, ImageFormat::RGBA16F, ImageFormat::Depth };
			geoFramebufferSpec.Samples = 1;
			geoFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			geoFramebufferSpec.DebugName = "Geometry";
			// TODO width and height
			geoFramebufferSpec.Width = 1600;
			geoFramebufferSpec.Height = 900;
			Ref<Framebuffer> framebuffer = Framebuffer::Create(geoFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			// pipelineSpecification.LineWidth = m_LineWidth;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = ShaderLibrary::Get("PBR_static");// Renderer::GetShaderLibrary()->Get("HazelPBR_Static");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			// renderPassSpec.DebugName = "Geometry";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			// pipelineSpecification.DebugName = "PBR-Static";
			m_GeometryPipeline = Pipeline::Create(pipelineSpecification);

			m_TestDiffuse = Texture2D::Create("assets/textures/FloorSandStone/cobblestone.png");
			m_TestSpecular = Texture2D::Create("assets/textures/FloorSandStone/cobblestone_SPEC.png");
			m_TestNormal = Texture2D::Create("assets/textures/FloorSandStone/cobblestone_NRM.png");
			// TODO: test remove
			m_testMaterial = Material::Create(ShaderLibrary::Get("PBR_static"), "tset Geo material");
			m_testMaterial->Set("u_DiffuseSampler", m_TestDiffuse);
			m_testMaterial->Set("u_SpecularSampler", m_TestSpecular);
			m_testMaterial->Set("u_NormalSampler", m_TestNormal);
			// VulkanRendererAPI::UpdateMaterialForRendering(std::dynamic_pointer_cast<VulkanMaterial>(m_testMaterial));
		}

		// Grid
		{
			m_GridShader = ShaderLibrary::Get("Grid");
			const float gridScale = 16.025f;
			const float gridSize = 0.025f;
			m_GridMaterial = Material::Create(m_GridShader);
			// m_GridMaterial->Set("u_Settings.Scale", gridScale);
			// m_GridMaterial->Set("u_Settings.Size", gridSize);
		
			PipelineSpecification pipelineSpec;
			// pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = m_GridShader;
			// pipelineSpec.BackfaceCulling = false;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = m_GeometryPipeline->GetSpecification().RenderPass;
			m_GridPipeline = Pipeline::Create(pipelineSpec);
		}

		// Composite
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			// compFramebufferSpec.SwapChainTarget = true;
			compFramebufferSpec.DebugName = "SceneComposite";
			// TODO: width and height
			compFramebufferSpec.Width = 1600;
			compFramebufferSpec.Height = 900;

			compFramebufferSpec.Attachments = { ImageFormat::RGBA };

			Ref<Framebuffer> framebuffer = Framebuffer::Create(compFramebufferSpec);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			// renderPassSpec.DebugName = "SceneComposite";

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			// pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Shader = ShaderLibrary::Get("SceneComposite");// Renderer::GetShaderLibrary()->Get("SceneComposite");
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			// pipelineSpecification.DebugName = "SceneComposite";
			// pipelineSpecification.DepthWrite = false;
			m_CompositePipeline = Pipeline::Create(pipelineSpecification);
			
			// TODO: test
			if (m_Texture2D == nullptr)
				m_Texture2D = Texture2D::Create("assets/textures/test.png");
			m_CompositeMaterial = Material::Create(ShaderLibrary::Get("SceneComposite"), "Composite material");
			
			auto geoFramebuffer = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
			m_CompositeMaterial->Set("u_Texture", std::dynamic_pointer_cast<VulkanFramebuffer>(geoFramebuffer)->GetImage());
		}

#endif 
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (m_ViewportWidth != width || m_ViewportHeight != height)
		{
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			m_InvViewportWidth = 1.f / (float)width;
			m_InvViewportHeight = 1.f / (float)height;
			m_NeedResize = true;
		}
	}

	void SceneRenderer::BeginScene(const Camera& camera, const glm::mat4& viewMatrix)
	{
		ETH_PROFILE_FUNCTION();

		if (m_NeedResize)
		{
			m_NeedResize = false;
		
			m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);

			auto geoFramebuffer = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
			m_CompositeMaterial->Set("u_Texture", std::dynamic_pointer_cast<VulkanFramebuffer>(geoFramebuffer)->GetImage());
		}


		// Update uniform buffers
		UBCamera& cameraData = CameraDataUB;

		auto& sceneCamera = camera;
		const auto viewProjection = sceneCamera.GetProjection() * viewMatrix;
		const glm::vec3 cameraPosition = glm::inverse(viewMatrix)[3];
		const auto inverseVP = glm::inverse(viewProjection);

		cameraData.ViewProjection = viewProjection;
		cameraData.InverseViewProjection = inverseVP;
		cameraData.Projection = sceneCamera.GetProjection();
		cameraData.View = viewMatrix;

		UBGlobal& globalData = m_GlobalUB;
		globalData.ViewProjection = viewProjection;
		globalData.AmbientColor = glm::vec4(0.8, 0.8, 0.8, 1);
		globalData.ViewPosition = cameraPosition;
		
		UBLocal localUB;
		localUB.DiffuseColor = glm::vec4(1, 1, 1, 1);
		localUB.Shininess = 32;

		VulkanRendererAPI::SetUniformBuffer(0, 0, &globalData, sizeof(globalData), 0);
		VulkanRendererAPI::SetUniformBuffer(0, 1, &localUB, sizeof(localUB), 0);
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> material)
	{
		// TODO: Culling, sorting, etc.
		m_DrawList.push_back({ mesh, transform, material });
		// m_ShadowPassDrawList.push_back({ mesh, materialTable, transform, overrideMaterial });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> material)
	{
		m_SelectedMeshDrawList.push_back({ mesh, transform, material });
		// m_ShadowPassDrawList.push_back({ mesh, materialTable, transform, overrideMaterial });
	}

	void SceneRenderer::EndScene()
	{
		ETH_PROFILE_FUNCTION();

		Flush();
	}

	void SceneRenderer::GeometryPass()
	{
		ETH_PROFILE_FUNCTION();

		// VulkanRendererAPI::BeginRenderPass(m_SelectedGeometryPipeline->GetSpecification().RenderPass);
		// for (auto& dc : m_SelectedMeshDrawList)
		// {
		// 	Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SelectedGeometryPipeline, m_UniformBufferSet, nullptr, dc.Mesh, dc.Transform, m_SelectedGeometryMaterial);
		// }
		// Renderer::EndRenderPass(m_CommandBuffer);

		VulkanRendererAPI::BeginRenderPass(m_GeometryPipeline->GetSpecification().RenderPass);
		// Skybox
		// m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SceneData.SkyboxLod);
		// m_SkyboxMaterial->Set("u_Uniforms.Intensity", m_SceneData.SceneEnvironmentIntensity);

		// const Ref<TextureCube> radianceMap = m_SceneData.SceneEnvironment ? m_SceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		// m_SkyboxMaterial->Set("u_Texture", radianceMap);
		// Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_SkyboxPipeline, m_UniformBufferSet, nullptr, m_SkyboxMaterial);

		// Render entities
		for (auto& dc : m_DrawList)
		{
			// VulkanRendererAPI::DrawMesh(m_GeometryPipeline, dc.Mesh, dc.Material, dc.Transform);
			VulkanRendererAPI::DrawMesh(m_GeometryPipeline, dc.Mesh, m_testMaterial, dc.Transform);
		}
		 // test VulkanRendererAPI::DrawMesh(m_GeometryPipeline, m_testMesh, m_testMaterial, glm::mat4(1.0f));

		// for (auto& dc : m_SelectedMeshDrawList)
		// {
		// 	Renderer::RenderMesh(m_CommandBuffer, m_GeometryPipeline, m_UniformBufferSet, m_StorageBufferSet, dc.Mesh, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), dc.Transform);
		// 	if (m_Options.ShowSelectedInWireframe)
		// 		Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_GeometryWireframePipeline, m_UniformBufferSet, nullptr, dc.Mesh, dc.Transform, m_WireframeMaterial);
		// }

		// Grid
		if (GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(8.0f));
			VulkanRendererAPI::DrawQuad(m_GridPipeline, m_GridMaterial, transform);
		}

		VulkanRendererAPI::EndRenderPass();
	}

	void SceneRenderer::CompositePass()
	{
		ETH_PROFILE_FUNCTION();

		VulkanRendererAPI::BeginRenderPass(m_CompositePipeline->GetSpecification().RenderPass, false);

		auto geoFramebuffer = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
		// float exposure = 0.5;// m_SceneData.SceneCamera.Camera.GetExposure();
		// int textureSamples = framebuffer->GetSpecification().Samples;

		// CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		// if (m_BloomSettings.Enabled)
		// {
		// 	CompositeMaterial->Set("u_Uniforms.BloomIntensity", m_BloomSettings.Intensity);
		// 	CompositeMaterial->Set("u_Uniforms.BloomDirtIntensity", m_BloomSettings.DirtIntensity);
		// }
		// else
		// {
		// 	CompositeMaterial->Set("u_Uniforms.BloomIntensity", 0.0f);
		// 	CompositeMaterial->Set("u_Uniforms.BloomDirtIntensity", 0.0f);
		// }
		// 
		// CompositeMaterial->Set("u_Uniforms.TextureSamples", textureSamples);

		// TODO:
		// m_CompositeMaterial->Set("u_Texture", std::dynamic_pointer_cast<VulkanFramebuffer>(framebuffer)->GetImage());
		// CompositeMaterial->Set("u_BloomTexture", m_BloomComputeTextures[2]);
		// CompositeMaterial->Set("u_BloomDirtTexture", m_BloomDirtTexture);

		// m_CompositeMaterial->Set("u_Texture", std::dynamic_pointer_cast<VulkanFramebuffer>(geoFramebuffer)->GetImage());

		VulkanRendererAPI::DrawFullscreenQuad(m_CompositePipeline, m_CompositeMaterial);
		// VulkanRendererAPI::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodCompositePipeline, nullptr, m_JumpFloodCompositeMaterial);
		VulkanRendererAPI::EndRenderPass();

		//Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodCompositePipeline->GetSpecification().RenderPass);
		//Renderer::EndRenderPass(m_CommandBuffer);

		// if (m_Options.ShowPhysicsColliders != SceneRendererOptions::PhysicsColliderView::None)
		// {
		// 	Renderer::BeginRenderPass(m_CommandBuffer, m_ExternalCompositeRenderPass);
		// 	auto pipeline = m_Options.ShowPhysicsColliders == SceneRendererOptions::PhysicsColliderView::Normal ? m_GeometryWireframePipeline : m_GeometryWireframeOnTopPipeline;
		// 	for (DrawCommand& dc : m_ColliderDrawList)
		// 	{
		// 		Renderer::RenderMeshWithMaterial(m_CommandBuffer, pipeline, m_UniformBufferSet, nullptr, dc.Mesh, dc.Transform, m_ColliderMaterial);
		// 	}
		// 	Renderer::EndRenderPass(m_CommandBuffer);
		// }
	}

	void SceneRenderer::Flush()
	{
		VulkanRendererAPI::BeginRenderCommandBuffer(m_CommandBuffer);

		GeometryPass();

		CompositePass();

		VulkanRendererAPI::EndRenderCommandBuffer();
		m_CommandBuffer->Submit();

		m_DrawList.clear();
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage()
	{
		return std::dynamic_pointer_cast<VulkanFramebuffer>(m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer)->GetImage();
	}
}