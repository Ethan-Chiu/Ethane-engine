#include "EditorLayer.h"
#include "DemoStele.h"
#include "Game/Spin.h"

#include <imgui.h>
#include <imgui_internal.h> // DockBuilder
#include <misc/cpp/imgui_stdlib.h>
#include <ImGuizmo.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Ethane/GfxBackend/Resource.h"
#include "Ethane/Renderer/RenderWork.h"

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
		viewportSpec.Width = m_ViewportSize.x;
		viewportSpec.Height = m_ViewportSize.y;
		viewportSpec.ClearColor = { 0.08f, 0.085f, 0.09f, 1.0f };
		m_ViewportTarget = backend.CreateRenderTarget(viewportSpec);

		std::string error;
		if (std::filesystem::exists(kAssetRegistryFile) && !m_AssetRegistry.Load(kAssetRegistryFile, &error)) {
			ETH_WARN("Couldn't read {}: {}", kAssetRegistryFile, error);
		}
		// Meshes and textures load on worker threads; the library uploads them as they finish (OnUpdate)
		m_Assets = CreateScope<AssetLibrary>(backend, m_AssetRegistry, &Application::Get().GetJobSystem());
		m_Renderer = CreateScope<Renderer>(backend, *m_Assets, m_ViewportTarget->GetRenderPass(), "assets/shaders/mesh.glsl", "assets/shaders/pick.glsl");
		m_GizmoOperation = ImGuizmo::TRANSLATE;
		// Dragging inside a window (e.g. a gizmo in the viewport) must not move the window
		ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
		m_Context = CreateScope<EditorContext>(*m_Assets, kAssetRegistryFile);

		// Game types first: steles that use them load only once they're registered
		RegisterGameTypes();

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
		Stop();
		m_Context.reset();
		m_Renderer.reset();
		m_Assets.reset();
		m_ViewportTarget.reset();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		m_Assets->Update();

		// A viewport click's pick, answered a frame or two later: select what was under the cursor
		if (std::optional<ElementId> picked = m_Renderer->TakePick(); picked && !IsPlaying()) {
			if (OpenStele* stele = m_Context->Active()) {
				stele->Selected = picked->IsValid() && stele->Source->Read().Contains(*picked) ? *picked : ElementId{};
			}
		}

		m_Camera.SetViewportSize(static_cast<float>(m_ViewportSize.x), static_cast<float>(m_ViewportSize.y));

		m_Camera.OnUpdate(ts, m_ViewportHovered && EditorCameraInControl());
		if (IsPlaying()) {
			m_PlayWorld->Update({ .DeltaTime = ts });
		} else if (OpenStele* stele = m_Context->Active()) {
			stele->EditWorld->Update({ .DeltaTime = ts });
		}
	}

	void EditorLayer::OnSubmitRender(RenderWork& work)
	{
		// The packet is built here, from the world and camera (state of this side), then moved into the pass
		FramePacket packet;
		packet.View = { m_Camera.GetViewMatrix(), m_Camera.GetProjectionMatrix(), m_Camera.GetPosition() };
		const World* world = IsPlaying() ? static_cast<const World*>(m_PlayWorld.get())
		                   : m_Context->Active() ? m_Context->Active()->EditWorld.get() : nullptr;
		if (world) {
			// Playing: through the stele's camera if it has one, else the editor camera
			const float aspect = static_cast<float>(m_ViewportSize.x) / std::max(1.0f, static_cast<float>(m_ViewportSize.y));
			m_PlayHasCamera = IsPlaying() && world->ExtractView(aspect, packet.View);
			world->Extract(packet);
		}

		// Recording owns the renderer and the target: the pass resizes the target to the panel's size (a no-op
		// when unchanged), then draws the packet into it
		work.Offscreen([renderer = m_Renderer.get(), target = m_ViewportTarget, size = m_ViewportSize,
		                packet = std::make_shared<FramePacket>(std::move(packet))](ICommandList& commandList) {
			if (size.x > 0 && size.y > 0) {
				target->Resize(size.x, size.y);
			}
			renderer->Render(commandList, *packet, *target);
		});
	}

	void EditorLayer::OnEvent(Event& e)
	{
		// Reaches here only when ImGui doesn't want the event, or the cursor is over the viewport
		if (EditorCameraInControl()) {
			m_Camera.OnEvent(e);
		}
	}

	void EditorLayer::OnImGuiRender()
	{
		ImGuizmo::BeginFrame();
		HandleShortcuts();
		DrawMenuBar();
		DrawDockspace();
		DrawViewport();
		if (m_ShowHierarchy) m_Hierarchy.Draw(*m_Context, &m_ShowHierarchy);
		if (m_ShowInspector) m_Inspector.Draw(*m_Context, &m_ShowInspector);
		if (m_ShowStats) DrawStatsPanel();
		if (m_ShowImGuiDemo) ImGui::ShowDemoWindow(&m_ShowImGuiDemo);
		DrawFileDialogs();

		// Play mode belongs to one stele: switching to another tab or closing it stops it
		if (IsPlaying() && m_Context->Active() != m_Playing) {
			Stop();
		}

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
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_P, global)) {
			IsPlaying() ? Stop() : Play();
		}
		// Gizmo operation: W / E / R while the cursor is over the viewport (not while typing)
		if (m_ViewportHovered && !IsPlaying() && !ImGui::GetIO().WantTextInput && !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeySuper) {
			if (ImGui::IsKeyPressed(ImGuiKey_W, false)) m_GizmoOperation = ImGuizmo::TRANSLATE;
			if (ImGui::IsKeyPressed(ImGuiKey_E, false)) m_GizmoOperation = ImGuizmo::ROTATE;
			if (ImGui::IsKeyPressed(ImGuiKey_R, false)) m_GizmoOperation = ImGuizmo::SCALE;
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
		if (ImGui::BeginMenu("Play")) {
			if (ImGui::MenuItem("Play", "Ctrl+P", false, !IsPlaying() && active)) {
				Play();
			}
			if (ImGui::MenuItem("Stop", "Ctrl+P", false, IsPlaying())) {
				Stop();
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

		DrawViewportToolbar();
		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		// Render at the panel's size in pixels (the framebuffer scale is 2 on Retina displays)
		const ImVec2 size = ImGui::GetContentRegionAvail();
		const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
		m_ViewportSize = { static_cast<uint32_t>(std::max(size.x * scale.x, 0.0f)), static_cast<uint32_t>(std::max(size.y * scale.y, 0.0f)) };

		if (size.x > 0.0f && size.y > 0.0f) {
			const ImVec2 min = ImGui::GetCursorScreenPos();
			ImGui::Image(Application::Get().GetImGuiLayer()->GetTextureID(*m_ViewportTarget), size);
			const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
			if (IsPlaying()) {
				// A frame around the image, so play mode can't be mistaken for editing
				ImGui::GetWindowDrawList()->AddRect(min, ImVec2(min.x + size.x, min.y + size.y), IM_COL32(90, 200, 120, 255), 0.0f, 0, 3.0f);
			} else {
				const bool onGizmo = DrawGizmo({ min.x, min.y }, { size.x, size.y });
				// A click on the scene (not on a gizmo, not Alt for the camera) selects what's under the cursor
				if (clicked && !onGizmo && !ImGui::GetIO().KeyAlt) {
					const ImVec2 mouse = ImGui::GetMousePos();
					const glm::uvec2 pixel{
						static_cast<uint32_t>(std::clamp((mouse.x - min.x) * scale.x, 0.0f, std::max(size.x * scale.x - 1.0f, 0.0f))),
						static_cast<uint32_t>(std::clamp((mouse.y - min.y) * scale.y, 0.0f, std::max(size.y * scale.y - 1.0f, 0.0f))) };
					m_Renderer->RequestPick(pixel);
				}
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorLayer::DrawViewportToolbar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 3.0f));
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 6.0f, ImGui::GetCursorPosY() + 4.0f));
		if (!IsPlaying()) {
			ImGui::BeginDisabled(m_Context->Active() == nullptr);
			if (ImGui::Button("Play")) {
				Play();
			}
			ImGui::EndDisabled();

			// Gizmo: operation (W / E / R) and space
			ImGui::SameLine(0.0f, 16.0f);
			auto operation = [&](const char* label, int value, const char* key) {
				const bool active = m_GizmoOperation == value;
				if (active) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				}
				if (ImGui::Button(label)) {
					m_GizmoOperation = value;
				}
				if (active) {
					ImGui::PopStyleColor();
				}
				ImGui::SetItemTooltip("%s", key);
				ImGui::SameLine();
			};
			operation("Move", ImGuizmo::TRANSLATE, "W");
			operation("Rotate", ImGuizmo::ROTATE, "E");
			operation("Scale", ImGuizmo::SCALE, "R");
			if (ImGui::Button(m_GizmoLocal ? "Local" : "World")) {
				m_GizmoLocal = !m_GizmoLocal;
			}
			ImGui::SetItemTooltip("Gizmo axes: the element's own (Local) or the world's. Hold Ctrl to snap.");
		} else {
			if (ImGui::Button("Stop")) {
				Stop();
			}
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			if (m_PlayHasCamera) {
				ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.55f, 1.0f), "Playing");
			} else {
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Playing (no primary camera: editor view)");
			}
		}
		ImGui::PopStyleVar();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
	}

	bool EditorLayer::DrawGizmo(glm::vec2 min, glm::vec2 size)
	{
		OpenStele* stele = m_Context->Active();
		if (!stele) {
			return false;
		}
		const SteleSnapshot snapshot = stele->Source->Read();
		const SteleElement* element = snapshot.Find(stele->Selected);
		const TransformComponent* transform = element ? snapshot.Get<TransformComponent>(element->Id) : nullptr;
		if (!transform) {
			m_GizmoWasUsing = false;
			return false;
		}

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(min.x, min.y, size.x, size.y);
		ImGuizmo::Enable(!ImGui::GetIO().KeyAlt); // Alt + drag is the camera

		// The gizmo edits the world matrix; the Transform is relative to the parent
		const glm::mat4 parentWorld = element->Parent.IsValid() ? WorldTransform(snapshot, element->Parent) : glm::mat4(1.0f);
		glm::mat4 world = parentWorld * transform->Matrix();
		const auto operation = static_cast<ImGuizmo::OPERATION>(m_GizmoOperation);
		const float snapValue = operation == ImGuizmo::ROTATE ? 15.0f : operation == ImGuizmo::SCALE ? 0.1f : 0.5f; // degrees / units
		const float snap[3] = { snapValue, snapValue, snapValue };
		const bool changed = ImGuizmo::Manipulate(glm::value_ptr(m_Camera.GetViewMatrix()), glm::value_ptr(m_Camera.GetProjectionMatrix()),
			operation, m_GizmoLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD, glm::value_ptr(world), nullptr, ImGui::GetIO().KeyCtrl ? snap : nullptr);

		// One drag is one undo step: a merge key per drag (top bit set: the Inspector's keys never have it)
		const bool using_ = ImGuizmo::IsUsing();
		if (using_ && !m_GizmoWasUsing) {
			m_GizmoGesture = (1ull << 63) | ++m_GizmoGestures;
		}
		m_GizmoWasUsing = using_;

		if (changed) {
			glm::vec3 scale, position, skew;
			glm::vec4 perspective;
			glm::quat rotation;
			if (glm::decompose(glm::inverse(parentWorld) * world, scale, rotation, position, skew, perspective)) {
				// Only what this operation changes, so the other fields don't pick up rounding noise
				Transaction tx(operation == ImGuizmo::ROTATE ? "Rotate " + element->Name : operation == ImGuizmo::SCALE ? "Scale " + element->Name : "Move " + element->Name);
				tx.MergeKey = m_GizmoGesture;
				if (operation == ImGuizmo::TRANSLATE) {
					tx.SetField<TransformComponent>(element->Id, "Position", position);
				} else if (operation == ImGuizmo::ROTATE) {
					tx.SetField<TransformComponent>(element->Id, "Rotation", EulerNear(rotation, transform->Rotation));
				} else {
					tx.SetField<TransformComponent>(element->Id, "Scale", scale);
				}
				m_Context->Commit(std::move(tx));
			}
		}
		return ImGuizmo::IsOver() || using_;
	}

	void EditorLayer::Play()
	{
		OpenStele* stele = m_Context->Active();
		if (!stele || IsPlaying()) {
			return;
		}
		// From the snapshot as it is now; then following the stele, so edits made while playing show up
		m_PlayWorld = CreateScope<EnttWorld>(stele->Source->Read(), WorldMode::Play);
		EnttWorld* world = m_PlayWorld.get();
		m_PlayFollow = stele->Source->Subscribe([world](const ChangeSet& changes) { world->Apply(changes); });
		m_Playing = stele;
		ETH_INFO("Play: {}", stele->Name);
	}

	void EditorLayer::Stop()
	{
		if (!IsPlaying()) {
			return;
		}
		m_PlayFollow.Reset();
		m_PlayWorld.reset(); // scripts get OnDestroy
		m_Playing = nullptr;
		m_PlayHasCamera = false;
		ETH_INFO("Stop");
	}

	void EditorLayer::DrawStatsPanel()
	{
		ImGui::Begin(kStatsWindow, &m_ShowStats);
		const ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / std::max(io.Framerate, 1e-3f));

		ImGui::Text("Viewport: %u x %u px", m_ViewportSize.x, m_ViewportSize.y);

		const RendererStats stats = m_Renderer->GetStats();
		ImGui::Text("Meshes: %u, draw calls: %u, triangles: %u", stats.DrawItems, stats.DrawCalls, stats.Triangles);
		if (const size_t loading = m_Assets->PendingCount(); loading > 0) {
			ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Loading %zu assets...", loading);
		} else if (stats.MissingMeshes > 0) {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%u meshes couldn't be loaded", stats.MissingMeshes);
		}

		if (OpenStele* stele = m_Context->Active()) {
			ImGui::SeparatorText("Stele");
			const SteleSnapshot snapshot = stele->Source->Read();
			ImGui::Text("%s%s", stele->Path.empty() ? "(not saved)" : stele->Path.generic_string().c_str(), stele->IsDirty() ? " *" : "");
			ImGui::Text("Version %llu, %zu elements", static_cast<unsigned long long>(snapshot.Version()), snapshot.ElementCount());
			ImGui::Text("Edit world: %zu entities, version %llu", stele->EditWorld->EntityCount(), static_cast<unsigned long long>(stele->EditWorld->Version()));
			if (IsPlaying()) {
				ImGui::Text("Play world: %zu entities, %zu scripts", m_PlayWorld->EntityCount(), m_PlayWorld->ScriptCount());
			}
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
		m_Camera.SetViewportSize(static_cast<float>(m_ViewportSize.x), static_cast<float>(m_ViewportSize.y));
	}

}
