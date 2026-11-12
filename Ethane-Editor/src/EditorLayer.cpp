#include "EditorLayer.h"
#include "DemoStele.h"

#include <imgui.h>
#include <imgui_internal.h> // DockBuilder
#include <misc/cpp/imgui_stdlib.h>

#include "Ethane/GfxBackend/Resource.h"

namespace Ethane {

	namespace {
		// Window names: ImGui identifies windows (and their docking state in imgui.ini) by them
		constexpr const char* kViewportWindow = "Viewport";
		constexpr const char* kStatsWindow = "Stats";
		constexpr const char* kOpenPopup = "Open stele";
		constexpr const char* kSaveAsPopup = "Save stele as";

		// Paths relative to the working directory (the editor runs in Ethane-Editor/)
		constexpr const char* kAssetRegistryFile = "assets/AssetRegistry.yaml";
		constexpr const char* kSteleFolder = "assets/steles";
		constexpr const char* kDemoStele = "assets/steles/Demo.stele";

		std::vector<std::string> SteleFiles()
		{
			std::vector<std::string> files;
			std::error_code ec;
			for (const auto& entry : std::filesystem::recursive_directory_iterator(kSteleFolder, ec))
				if (entry.is_regular_file() && entry.path().extension() == ".stele")
					files.push_back(entry.path().generic_string());
			std::sort(files.begin(), files.end());
			return files;
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
		m_Context = CreateScope<EditorContext>(*m_Assets, kAssetRegistryFile);

		// The demo stele, created and saved the first time
		if (!m_Context->Open(kDemoStele, error)) {
			ETH_WARN("Couldn't open {} ({}): building it", kDemoStele, error);
			NewDemoStele();
			if (!m_Context->Save(*m_Context->Active(), kDemoStele, error)) {
				ETH_WARN("Couldn't save {}: {}", kDemoStele, error);
			}
		}
		ResetCamera();
	}

	void EditorLayer::OnDetach()
	{
		m_Context.reset();
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
		if (OpenStele* stele = m_Context->Active()) {
			stele->EditWorld->Update({ .DeltaTime = ts });
		}
	}

	void EditorLayer::OnRenderOffscreen(ICommandList& commandList)
	{
		m_Packet.Clear();
		m_Packet.View = { m_Camera.GetViewMatrix(), m_Camera.GetProjectionMatrix(), m_Camera.GetPosition() };
		if (OpenStele* stele = m_Context->Active()) {
			stele->EditWorld->Extract(m_Packet);
		}
		m_Renderer->Render(commandList, m_Packet, *m_ViewportTarget);
	}

	void EditorLayer::OnEvent(Event& e)
	{
		// Reaches here only when ImGui doesn't want the event, or the cursor is over the viewport
		m_Camera.OnEvent(e);
	}

	void EditorLayer::OnImGuiRender()
	{
		HandleShortcuts();
		DrawMenuBar();
		DrawDockspace();
		DrawViewport();
		if (m_ShowHierarchy) m_Hierarchy.Draw(*m_Context, &m_ShowHierarchy);
		if (m_ShowInspector) m_Inspector.Draw(*m_Context, &m_ShowInspector);
		if (m_ShowStats) DrawStatsPanel();
		if (m_ShowImGuiDemo) ImGui::ShowDemoWindow(&m_ShowImGuiDemo);
		DrawFileDialogs();

		// Mouse / keyboard over the viewport go to the camera, everything else to ImGui
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);
	}

	void EditorLayer::HandleShortcuts()
	{
		// ImGuiMod_Ctrl is Cmd on macOS. Global routes: a focused text field keeps its own Ctrl+Z.
		const ImGuiInputFlags global = ImGuiInputFlags_RouteGlobal;
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, global)) {
			m_Context->Undo();
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, global) || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, global)) {
			m_Context->Redo();
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, global)) {
			Save(false);
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, global)) {
			Save(true);
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, global)) {
			m_OpenRequested = true;
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, global)) {
			NewStele();
		}
	}

	void EditorLayer::DrawMenuBar()
	{
		if (!ImGui::BeginMainMenuBar()) {
			return;
		}
		OpenStele* active = m_Context->Active();
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) {
				NewStele();
			}
			if (ImGui::MenuItem("New demo stele")) {
				NewDemoStele();
			}
			if (ImGui::MenuItem("Open...", "Ctrl+O")) {
				m_OpenRequested = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Save", "Ctrl+S", false, active != nullptr)) {
				Save(false);
			}
			if (ImGui::MenuItem("Save as...", "Shift+Ctrl+S", false, active != nullptr)) {
				Save(true);
			}
			if (ImGui::MenuItem("Close", nullptr, false, active != nullptr)) {
				m_Context->Close(m_Context->ActiveIndex());
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) {
				Application::Get().Close();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			const std::string undo = active && active->History.CanUndo() ? "Undo " + active->History.UndoLabel() : "Undo";
			const std::string redo = active && active->History.CanRedo() ? "Redo " + active->History.RedoLabel() : "Redo";
			if (ImGui::MenuItem(undo.c_str(), "Ctrl+Z", false, active && active->History.CanUndo())) {
				m_Context->Undo();
			}
			if (ImGui::MenuItem(redo.c_str(), "Shift+Ctrl+Z", false, active && active->History.CanRedo())) {
				m_Context->Redo();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem(HierarchyPanel::kWindowName, nullptr, &m_ShowHierarchy);
			ImGui::MenuItem(InspectorPanel::kWindowName, nullptr, &m_ShowInspector);
			ImGui::MenuItem(kStatsWindow, nullptr, &m_ShowStats);
			ImGui::Separator();
			if (ImGui::MenuItem("Reset camera")) {
				ResetCamera();
			}
			if (ImGui::MenuItem("Reset layout")) {
				m_ResetLayout = true;
				m_ShowHierarchy = m_ShowInspector = m_ShowStats = true;
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
			ImGui::DockBuilderDockWindow(HierarchyPanel::kWindowName, leftTop);
			ImGui::DockBuilderDockWindow(kStatsWindow, leftBottom);
			ImGui::DockBuilderDockWindow(InspectorPanel::kWindowName, right);
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

		if (OpenStele* stele = m_Context->Active()) {
			ImGui::SeparatorText("Stele");
			const SteleSnapshot snapshot = stele->Source->Read();
			ImGui::Text("%s%s", stele->Path.empty() ? "(not saved)" : stele->Path.generic_string().c_str(), stele->IsDirty() ? " *" : "");
			ImGui::Text("Version %llu, %zu elements", static_cast<unsigned long long>(snapshot.Version()), snapshot.ElementCount());
			ImGui::Text("Edit world: %zu entities, version %llu", stele->EditWorld->EntityCount(), static_cast<unsigned long long>(stele->EditWorld->Version()));
		}

		ImGui::SeparatorText("Camera");
		ImGui::Text("Distance %.2f, pitch %.2f, yaw %.2f", m_Camera.GetDistance(), m_Camera.GetPitch(), m_Camera.GetYaw());
		if (ImGui::Button("Reset camera")) {
			ResetCamera();
		}

		ImGui::SeparatorText("About");
		ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
		ImGui::End();
	}

	void EditorLayer::DrawFileDialogs()
	{
		// Path popups (there is no native file dialog yet)
		if (m_OpenRequested) {
			m_OpenRequested = false;
			m_DialogPath = kSteleFolder + std::string("/");
			m_DialogError.clear();
			ImGui::OpenPopup(kOpenPopup);
		}
		if (m_SaveAsRequested) {
			m_SaveAsRequested = false;
			const OpenStele* active = m_Context->Active();
			m_DialogPath = active && !active->Path.empty() ? active->Path.generic_string() : std::string(kSteleFolder) + "/" + (active ? active->Name : "Untitled") + ".stele";
			m_DialogError.clear();
			ImGui::OpenPopup(kSaveAsPopup);
		}

		const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(kOpenPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			std::optional<std::string> open;
			for (const std::string& file : SteleFiles()) {
				if (ImGui::Selectable(file.c_str(), false, ImGuiSelectableFlags_DontClosePopups)) {
					open = file;
				}
			}
			ImGui::Separator();
			ImGui::SetNextItemWidth(420.0f);
			if (ImGui::InputText("Path", &m_DialogPath, ImGuiInputTextFlags_EnterReturnsTrue)) {
				open = m_DialogPath;
			}
			if (ImGui::Button("Open")) {
				open = m_DialogPath;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				ImGui::CloseCurrentPopup();
			}
			if (open) {
				if (m_Context->Open(*open, m_DialogError)) {
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_DialogError.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "%s", m_DialogError.c_str());
			}
			ImGui::EndPopup();
		}

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(kSaveAsPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::SetNextItemWidth(420.0f);
			bool save = ImGui::InputText("Path", &m_DialogPath, ImGuiInputTextFlags_EnterReturnsTrue);
			save |= ImGui::Button("Save");
			ImGui::SameLine();
			if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				ImGui::CloseCurrentPopup();
			}
			if (save) {
				OpenStele* active = m_Context->Active();
				if (active && m_Context->Save(*active, m_DialogPath, m_DialogError)) {
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_DialogError.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "%s", m_DialogError.c_str());
			}
			ImGui::EndPopup();
		}
	}

	void EditorLayer::NewStele()
	{
		OpenStele& stele = m_Context->New("Untitled");
		Transaction tx("Create sun");
		const ElementId sun = tx.CreateElement("Sun");
		tx.AddComponent(sun, TransformComponent{ .Rotation = RotationTowards({ -0.4f, -0.8f, -0.6f }) });
		tx.AddComponent(sun, DirectionalLightComponent{});
		stele.Source->Commit(std::move(tx)); // part of the new stele, not an undoable edit
		stele.SavedVersion = stele.Source->Read().Version();
	}

	void EditorLayer::NewDemoStele()
	{
		OpenStele& stele = m_Context->New("Demo");
		BuildDemoStele(*stele.Source, *m_Assets);
		stele.SavedVersion = stele.Source->Read().Version();
	}

	void EditorLayer::Save(bool chooseFile)
	{
		OpenStele* active = m_Context->Active();
		if (!active) {
			return;
		}
		if (chooseFile || active->Path.empty()) {
			m_SaveAsRequested = true;
			return;
		}
		std::string error;
		if (!m_Context->Save(*active, {}, error)) {
			ETH_ERROR("Save failed: {}", error);
		}
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
