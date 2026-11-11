#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h> // DockBuilder

#include <glm/gtc/type_ptr.hpp>

#include "Ethane/GfxBackend/Resource.h"

namespace Ethane {

	namespace {
		// Window names: ImGui identifies windows (and their docking state in imgui.ini) by them
		constexpr const char* kViewportWindow = "Viewport";
		constexpr const char* kSceneWindow = "Scene";
		constexpr const char* kPropertiesWindow = "Properties";
		constexpr const char* kStatsWindow = "Stats";

		uint32_t CountTriangles(const Mesh& mesh)
		{
			uint32_t indices = 0;
			for (const Submesh& submesh : mesh.GetSubmeshes()) {
				indices += submesh.IndexCount;
			}
			return indices / 3;
		}
	}

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
	}

	void EditorLayer::OnAttach()
	{
		{
			// Memory resource experiment: routes std::pmr allocations through a linear resource, reporting
			// to new/delete. The default resource is restored before these locals go out of scope.
			ReportMemResource report_new{ "NewDelete", NewDeleteMemResource() };
			LinearMemResource linear_new{ &report_new };
			PMRAdapter alloc(linear_new);
			std::pmr::memory_resource* previous = std::pmr::set_default_resource(&alloc);

			std::pmr::vector<int> testAlloc;
			for (uint32_t i = 0; i < 50; i++) {
				testAlloc.push_back(i);
			}
			std::pmr::set_default_resource(previous);
		}

		GfxBackendAPI& backend = Application::Get().GetGfxBackend();

		RenderTargetSpecification viewportSpec;
		viewportSpec.Width = 1280;
		viewportSpec.Height = 720;
		viewportSpec.ClearColor = { 0.08f, 0.085f, 0.09f, 1.0f };
		m_ViewportTarget = backend.CreateRenderTarget(viewportSpec);

		m_Scene = CreateScope<DemoScene>(backend, m_ViewportTarget->GetRenderPass());
		ResetCamera();
	}

	void EditorLayer::OnDetach()
	{
		m_Scene.reset();
		m_ViewportTarget.reset();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		// The viewport panel's size from the last UI frame. Applied here, before the UI asks for the image
		// (resizing replaces it): the panel shows the new size from this frame on.
		const RenderTargetSpecification& spec = m_ViewportTarget->GetSpecification();
		if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0 && (m_ViewportSize.x != spec.Width || m_ViewportSize.y != spec.Height)) {
			m_ViewportTarget->Resize(m_ViewportSize.x, m_ViewportSize.y);
		}
		m_Camera.SetViewportSize(static_cast<float>(spec.Width), static_cast<float>(spec.Height));

		if (m_ViewportHovered) {
			m_Camera.OnUpdate(ts);
		}
		m_Scene->OnUpdate(ts);
	}

	void EditorLayer::OnRenderOffscreen(ICommandList& commandList)
	{
		commandList.BeginRenderPass(*m_ViewportTarget);
		m_Scene->Render(commandList, m_Camera);
		commandList.EndRenderPass();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		// Reaches here only when ImGui doesn't want the event, or the cursor is over the viewport
		m_Camera.OnEvent(e);
	}

	void EditorLayer::OnImGuiRender()
	{
		DrawMenuBar();
		DrawDockspace();
		DrawViewport();
		if (m_ShowScene) DrawScenePanel();
		if (m_ShowProperties) DrawPropertiesPanel();
		if (m_ShowStats) DrawStatsPanel();
		if (m_ShowImGuiDemo) ImGui::ShowDemoWindow(&m_ShowImGuiDemo);

		// Mouse / keyboard over the viewport go to the camera, everything else to ImGui
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);
	}

	void EditorLayer::DrawMenuBar()
	{
		if (!ImGui::BeginMainMenuBar()) {
			return;
		}
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) {
				Application::Get().Close();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem(kSceneWindow, nullptr, &m_ShowScene);
			ImGui::MenuItem(kPropertiesWindow, nullptr, &m_ShowProperties);
			ImGui::MenuItem(kStatsWindow, nullptr, &m_ShowStats);
			ImGui::Separator();
			if (ImGui::MenuItem("Reset camera")) {
				ResetCamera();
			}
			if (ImGui::MenuItem("Reset layout")) {
				m_ResetLayout = true;
				m_ShowScene = m_ShowProperties = m_ShowStats = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			ImGui::MenuItem("ImGui demo window", nullptr, &m_ShowImGuiDemo);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	void EditorLayer::DrawDockspace()
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImGuiID dockspace = ImGui::GetID("EditorDockspace");

		// Default layout: on the first run (nothing in imgui.ini yet) or when asked to reset
		if (m_ResetLayout || !ImGui::DockBuilderGetNode(dockspace)) {
			m_ResetLayout = false;
			ImGui::DockBuilderRemoveNode(dockspace);
			ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace, viewport->WorkSize);

			ImGuiID center = dockspace;
			const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
			const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
			ImGuiID leftTop = left;
			const ImGuiID leftBottom = ImGui::DockBuilderSplitNode(leftTop, ImGuiDir_Down, 0.4f, nullptr, &leftTop);

			ImGui::DockBuilderDockWindow(kViewportWindow, center);
			ImGui::DockBuilderDockWindow(kSceneWindow, leftTop);
			ImGui::DockBuilderDockWindow(kStatsWindow, leftBottom);
			ImGui::DockBuilderDockWindow(kPropertiesWindow, right);
			ImGui::DockBuilderFinish(dockspace);
		}
		ImGui::DockSpaceOverViewport(dockspace, viewport);
	}

	void EditorLayer::DrawViewport()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(kViewportWindow);

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		// Render at the panel's size in pixels (the framebuffer scale is 2 on Retina displays)
		const ImVec2 size = ImGui::GetContentRegionAvail();
		const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
		m_ViewportSize = { static_cast<uint32_t>(std::max(size.x * scale.x, 0.0f)), static_cast<uint32_t>(std::max(size.y * scale.y, 0.0f)) };

		if (size.x > 0.0f && size.y > 0.0f) {
			ImGui::Image(Application::Get().GetImGuiLayer()->GetTextureID(*m_ViewportTarget), size);
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorLayer::DrawScenePanel()
	{
		ImGui::Begin(kSceneWindow, &m_ShowScene);
		std::vector<DemoScene::Model>& models = m_Scene->GetModels();
		for (int i = 0; i < static_cast<int>(models.size()); i++) {
			DemoScene::Model& model = models[i];
			ImGui::PushID(i);
			ImGui::Checkbox("##visible", &model.Visible);
			ImGui::SameLine();
			if (ImGui::Selectable(model.Name.c_str(), m_SelectedModel == i)) {
				m_SelectedModel = i;
			}
			ImGui::PopID();
		}
		// Clicking empty space clears the selection
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
			m_SelectedModel = -1;
		}
		ImGui::End();
	}

	void EditorLayer::DrawPropertiesPanel()
	{
		ImGui::Begin(kPropertiesWindow, &m_ShowProperties);
		std::vector<DemoScene::Model>& models = m_Scene->GetModels();
		if (m_SelectedModel < 0 || m_SelectedModel >= static_cast<int>(models.size())) {
			ImGui::TextDisabled("Select a model in the Scene panel");
			ImGui::End();
			return;
		}

		DemoScene::Model& model = models[m_SelectedModel];
		ImGui::SeparatorText(model.Name.c_str());
		ImGui::DragFloat3("Position", glm::value_ptr(model.Position), 0.05f);
		ImGui::DragFloat("Scale", &model.Scale, 0.01f, 0.01f, 100.0f);
		ImGui::Checkbox("Spin", &model.Spin);
		ImGui::SameLine();
		ImGui::Checkbox("Visible", &model.Visible);

		ImGui::SeparatorText("Materials");
		for (size_t i = 0; i < model.Materials.size(); i++) {
			Material& material = *model.Materials[i];
			MaterialData data = material.GetData();
			ImGui::PushID(static_cast<int>(i));
			const std::string label = data.Name.empty() ? "Material " + std::to_string(i) : data.Name;
			if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				bool changed = false;
				changed |= ImGui::ColorEdit4("Base color", glm::value_ptr(data.BaseColor));
				changed |= ImGui::SliderFloat("Metallic", &data.Metallic, 0.0f, 1.0f);
				changed |= ImGui::SliderFloat("Roughness", &data.Roughness, 0.05f, 1.0f);
				if (changed) {
					material.SetData(data);
				}
				auto textureRow = [](const char* name, const RefCountPtr<ITexture2D>& texture) {
					if (texture) {
						const TextureSpecification& spec = texture->GetSpecification();
						ImGui::Text("%s: %u x %u", name, spec.Width, spec.Height);
					} else {
						ImGui::TextDisabled("%s: none", name);
					}
				};
				textureRow("Base color map", data.BaseColorTexture);
				textureRow("Normal map", data.NormalTexture);
				textureRow("Metallic-roughness map", data.MetallicRoughnessTexture);
			}
			ImGui::PopID();
		}
		ImGui::End();
	}

	void EditorLayer::DrawStatsPanel()
	{
		ImGui::Begin(kStatsWindow, &m_ShowStats);
		const ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / std::max(io.Framerate, 1e-3f));

		const RenderTargetSpecification& spec = m_ViewportTarget->GetSpecification();
		ImGui::Text("Viewport: %u x %u px", spec.Width, spec.Height);

		uint32_t triangles = 0, visible = 0;
		for (const DemoScene::Model& model : m_Scene->GetModels()) {
			if (model.Visible) {
				visible++;
				triangles += CountTriangles(*model.MeshData);
			}
		}
		ImGui::Text("Models: %u visible, %u triangles", visible, triangles);

		ImGui::SeparatorText("Camera");
		ImGui::Text("Distance %.2f, pitch %.2f, yaw %.2f", m_Camera.GetDistance(), m_Camera.GetPitch(), m_Camera.GetYaw());
		if (ImGui::Button("Reset camera")) {
			ResetCamera();
		}
		ImGui::SliderFloat("Spin speed", &m_Scene->SpinSpeed, 0.0f, 3.0f, "%.2f rad/s");

		ImGui::SeparatorText("About");
		ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
		ImGui::End();
	}

	void EditorLayer::ResetCamera()
	{
		m_Camera = EditorCamera(45.0f, 1.778f, 0.1f, 100.0f);
		m_Camera.SetDistance(9.0f);
		m_Camera.SetPitchYaw(0.45f, 0.0f);
		const RenderTargetSpecification& spec = m_ViewportTarget->GetSpecification();
		m_Camera.SetViewportSize(static_cast<float>(spec.Width), static_cast<float>(spec.Height));
	}

}
