#include "EditorLayer.h"
#include "DemoStele.h"

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

		// Where the asset ids used by the editor's steles are kept (relative to the working directory)
		constexpr const char* kAssetRegistryFile = "assets/AssetRegistry.yaml";

		// A field edit as one transaction (undo, and merging a drag into one undo step, come with step 5)
		template<typename T>
		Transaction FieldEdit(ElementId id, std::string_view field, FieldValue value)
		{
			Transaction tx(fmt::format("Edit {}.{}", TypeRegistry::Global().Find<T>()->Name, field));
			tx.SetField<T>(id, field, std::move(value));
			return tx;
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

		std::string error;
		if (std::filesystem::exists(kAssetRegistryFile) && !m_AssetRegistry.Load(kAssetRegistryFile, &error)) {
			ETH_WARN("Couldn't read {}: {}", kAssetRegistryFile, error);
		}
		m_Assets = CreateScope<AssetLibrary>(backend, m_AssetRegistry);
		m_Renderer = CreateScope<Renderer>(backend, *m_Assets, m_ViewportTarget->GetRenderPass(), "assets/shaders/mesh.glsl");

		m_Stele = CreateScope<Stele>();
		BuildDemoStele(*m_Stele, *m_Assets);
		if (m_AssetRegistry.IsDirty() && !m_AssetRegistry.Save(kAssetRegistryFile)) {
			ETH_WARN("Couldn't save {}", kAssetRegistryFile);
		}

		// The edit world mirrors the stele: built from its snapshot, then kept up to date by its change sets
		m_EditWorld = CreateScope<EnttWorld>(m_Stele->Read());
		m_EditWorldFollow = m_Stele->Subscribe([this](const ChangeSet& changes) { m_EditWorld->Apply(changes); });
		ResetCamera();
	}

	void EditorLayer::OnDetach()
	{
		m_EditWorldFollow.Reset();
		m_EditWorld.reset();
		m_Stele.reset();
		m_Packet.Clear();
		m_Renderer.reset();
		m_Assets.reset();
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
		m_EditWorld->Update({ .DeltaTime = ts });
	}

	void EditorLayer::OnRenderOffscreen(ICommandList& commandList)
	{
		m_Packet.Clear();
		m_Packet.View = { m_Camera.GetViewMatrix(), m_Camera.GetProjectionMatrix(), m_Camera.GetPosition() };
		m_EditWorld->Extract(m_Packet);
		m_Renderer->Render(commandList, m_Packet, *m_ViewportTarget);
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
		const SteleSnapshot snapshot = m_Stele->Read();
		if (m_Selected.IsValid() && !snapshot.Contains(m_Selected)) {
			m_Selected = {};
		}
		for (ElementId id : snapshot.Children()) {
			DrawElementNode(snapshot, id);
		}
		// Clicking empty space clears the selection
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
			m_Selected = {};
		}
		ImGui::End();
	}

	void EditorLayer::DrawElementNode(const SteleSnapshot& snapshot, ElementId id)
	{
		const SteleElement& element = *snapshot.Find(id);
		const auto children = snapshot.Children(id);

		ImGui::PushID(static_cast<int>(id.Value ^ (id.Value >> 32)));
		if (const MeshComponent* mesh = snapshot.Get<MeshComponent>(id)) {
			bool visible = mesh->Visible;
			if (ImGui::Checkbox("##visible", &visible)) {
				Commit(FieldEdit<MeshComponent>(id, "Visible", visible));
			}
			ImGui::SameLine();
		}

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
		if (children.empty()) {
			flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (m_Selected == id) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		const bool open = ImGui::TreeNodeEx(element.Name.empty() ? "(unnamed)" : element.Name.c_str(), flags);
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			m_Selected = id;
		}
		if (open) {
			for (ElementId child : children) {
				DrawElementNode(snapshot, child);
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void EditorLayer::DrawPropertiesPanel()
	{
		ImGui::Begin(kPropertiesWindow, &m_ShowProperties);
		const SteleSnapshot snapshot = m_Stele->Read();
		const SteleElement* element = snapshot.Find(m_Selected);
		if (!element) {
			ImGui::TextDisabled("Select an element in the Scene panel");
			ImGui::End();
			return;
		}
		const ElementId id = element->Id;
		ImGui::SeparatorText(element->Name.c_str());

		// Hand-written widgets for the engine components for now; a schema-driven inspector replaces
		// them (roadmap step 5). Every change is a transaction: the edit world picks it up from the stele.
		if (const TransformComponent* transform = snapshot.Get<TransformComponent>(id)) {
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
				glm::vec3 position = transform->Position;
				glm::vec3 degrees = glm::degrees(transform->Rotation);
				glm::vec3 scale = transform->Scale;
				if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.05f)) {
					Commit(FieldEdit<TransformComponent>(id, "Position", position));
				}
				if (ImGui::DragFloat3("Rotation", glm::value_ptr(degrees), 0.5f)) {
					Commit(FieldEdit<TransformComponent>(id, "Rotation", glm::radians(degrees)));
				}
				if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f)) {
					Commit(FieldEdit<TransformComponent>(id, "Scale", scale));
				}
			}
		}
		if (const MeshComponent* mesh = snapshot.Get<MeshComponent>(id)) {
			if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
				auto path = m_AssetRegistry.GetPath(mesh->Mesh);
				ImGui::TextWrapped("%s", path ? path->generic_string().c_str() : "(no mesh)");
				bool visible = mesh->Visible;
				if (ImGui::Checkbox("Visible", &visible)) {
					Commit(FieldEdit<MeshComponent>(id, "Visible", visible));
				}
			}
		}
		if (const MaterialComponent* material = snapshot.Get<MaterialComponent>(id)) {
			if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
				glm::vec4 color = material->BaseColor;
				float metallic = material->Metallic;
				float roughness = material->Roughness;
				if (ImGui::ColorEdit4("Base color", glm::value_ptr(color))) {
					Commit(FieldEdit<MaterialComponent>(id, "BaseColor", color));
				}
				if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
					Commit(FieldEdit<MaterialComponent>(id, "Metallic", metallic));
				}
				if (ImGui::SliderFloat("Roughness", &roughness, 0.05f, 1.0f)) {
					Commit(FieldEdit<MaterialComponent>(id, "Roughness", roughness));
				}
				auto texture = m_AssetRegistry.GetPath(material->BaseColorTexture);
				ImGui::TextDisabled("Base color map: %s", texture ? texture->generic_string().c_str() : "the mesh's own");
			}
		}
		if (const DirectionalLightComponent* light = snapshot.Get<DirectionalLightComponent>(id)) {
			if (ImGui::CollapsingHeader("Directional light", ImGuiTreeNodeFlags_DefaultOpen)) {
				glm::vec3 color = light->Color;
				float intensity = light->Intensity;
				if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
					Commit(FieldEdit<DirectionalLightComponent>(id, "Color", color));
				}
				if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 3.0f)) {
					Commit(FieldEdit<DirectionalLightComponent>(id, "Intensity", intensity));
				}
				ImGui::TextDisabled("Aim it with the transform's rotation");
			}
		}
		ImGui::End();
	}

	void EditorLayer::Commit(Transaction transaction)
	{
		CommitResult result = m_Stele->Commit(std::move(transaction));
		if (!result) {
			ETH_WARN("Edit refused: {}", result.Error);
		}
	}

	void EditorLayer::DrawStatsPanel()
	{
		ImGui::Begin(kStatsWindow, &m_ShowStats);
		const ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / std::max(io.Framerate, 1e-3f));

		const RenderTargetSpecification& spec = m_ViewportTarget->GetSpecification();
		ImGui::Text("Viewport: %u x %u px", spec.Width, spec.Height);

		const RendererStats& stats = m_Renderer->GetStats();
		ImGui::Text("Meshes: %u, draw calls: %u, triangles: %u", stats.DrawItems, stats.DrawCalls, stats.Triangles);
		if (stats.MissingMeshes > 0) {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%u meshes couldn't be loaded", stats.MissingMeshes);
		}

		ImGui::SeparatorText("Stele");
		const SteleSnapshot snapshot = m_Stele->Read();
		ImGui::Text("Version %llu, %zu elements", static_cast<unsigned long long>(snapshot.Version()), snapshot.ElementCount());
		ImGui::Text("Edit world: %zu entities, version %llu", m_EditWorld->EntityCount(), static_cast<unsigned long long>(m_EditWorld->Version()));

		ImGui::SeparatorText("Camera");
		ImGui::Text("Distance %.2f, pitch %.2f, yaw %.2f", m_Camera.GetDistance(), m_Camera.GetPitch(), m_Camera.GetYaw());
		if (ImGui::Button("Reset camera")) {
			ResetCamera();
		}

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
