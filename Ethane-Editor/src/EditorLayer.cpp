#include "EditorLayer.h"
#include "DemoStele.h"
#include "Game/Spin.h"
#include "EditorActions.h"
#include "Panels/AssetDrop.h"
#include "Panels/SteleTabs.h"
#include "Theme/ImGuiTheme.h"
#include "Theme/ImGuiWidgets.h"

#include <utility>

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
		m_OutlineTarget = backend.CreateRenderTarget(RenderTargetSpecification{
			.Width = viewportSpec.Width, .Height = viewportSpec.Height, .ColorFormat = ImageFormat::RGBA,
			.DepthFormat = ImageFormat::None, .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f } });

		std::string error;
		if (std::filesystem::exists(kAssetRegistryFile) && !m_AssetRegistry.Load(kAssetRegistryFile, &error)) {
			ETH_WARN("Couldn't read {}: {}", kAssetRegistryFile, error);
		}
		// Meshes and textures load on worker threads; the library uploads them as they finish (OnUpdate)
		m_Assets = CreateScope<AssetLibrary>(backend, m_AssetRegistry, &Application::Get().GetJobSystem());
		m_Renderer = CreateScope<Renderer>(backend, *m_Assets, m_ViewportTarget->GetRenderPass()); // shaders from assets/shaders
		m_GizmoOperation = ImGuizmo::TRANSLATE;
		// Dragging inside a window (e.g. a gizmo in the viewport) must not move the window
		ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
		LoadImGuiFonts("assets/fonts");
		ApplyImGuiTheme(m_Theme);
		m_Context = CreateScope<EditorContext>(*m_Assets, kAssetRegistryFile);
		m_Content = CreateScope<ContentBrowser>(m_AssetRegistry, std::vector<std::filesystem::path>{ "assets", "res" });
		m_ContentPanel = CreateScope<ContentBrowserPanel>(*m_Content, *m_Assets, m_Theme);

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
		m_OutlineTarget.reset();
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

		// Editing: the selection (and everything under it) gets an outline
		if (OpenStele* stele = m_Context->Active(); stele && !IsPlaying() && stele->Selected.IsValid()) {
			const SteleSnapshot snapshot = stele->Source->Read();
			if (snapshot.Contains(stele->Selected)) {
				packet.Highlighted.push_back(stele->Selected);
				snapshot.ForEachDepthFirst([&](const SteleElement& element) { packet.Highlighted.push_back(element.Id); }, stele->Selected);
			}
		}

		// Recording owns the renderer and the targets: the pass resizes them to the panel's size (a no-op when
		// unchanged), draws the packet, then the selection outline over it
		const int outlineWidth = static_cast<int>(std::lround(2.0f * ImGui::GetIO().DisplayFramebufferScale.x)); // 2 points
		work.Offscreen([renderer = m_Renderer.get(), target = m_ViewportTarget, outline = m_OutlineTarget, size = m_ViewportSize,
		                outlineWidth, selection = m_Theme.Accent, packet = std::make_shared<FramePacket>(std::move(packet))](ICommandList& commandList) {
			if (size.x > 0 && size.y > 0) {
				target->Resize(size.x, size.y);
				outline->Resize(size.x, size.y);
			}
			renderer->Render(commandList, *packet, *target);
			renderer->RenderOutline(commandList, *packet, target, *outline, OutlineStyle{ .Color = selection, .Width = std::max(outlineWidth, 1) });
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
		if (m_ShowHierarchy) m_Hierarchy.Draw(*m_Context, &m_ShowHierarchy, m_Theme);
		if (m_ShowInspector) m_Inspector.Draw(*m_Context, &m_ShowInspector, m_Theme);
		if (m_ShowStats) DrawStatsPanel();
		if (m_ShowContent) {
			m_ContentPanel->Draw(&m_ShowContent, [this](const std::filesystem::path& path) {
				std::string error;
				if (!m_Context->Open(path, error)) {
					ETH_WARN("Couldn't open {}: {}", path.generic_string(), error);
				}
			});
		}
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
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, ImGui::GetStyle().ItemSpacing.y));
		const bool open = ImGui::BeginMainMenuBar();
		ImGui::PopStyleVar(2);
		if (!open) {
			return;
		}
		{
			// The mark: three bars, the middle one short
			const ImVec2 at = ImGui::GetCursorScreenPos();
			const float h = ImGui::GetFrameHeight(), y = at.y + h * 0.5f;
			ImDrawList* draw = ImGui::GetWindowDrawList();
			const ImU32 mark = ToImGuiU32(m_Theme.Accent);
			for (const auto& [dy, w] : { std::pair{ -4.0f, 10.0f }, std::pair{ 0.0f, 7.0f }, std::pair{ 4.0f, 10.0f } }) {
				draw->AddLine(ImVec2(at.x + 6.0f, y + dy), ImVec2(at.x + 6.0f + w, y + dy), mark, 2.0f);
			}
			ImGui::Dummy(ImVec2(24.0f, h * 0.5f));
			ImGui::SameLine();
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
			ImGui::MenuItem(ContentBrowserPanel::kWindowName, nullptr, &m_ShowContent);
			ImGui::Separator();
			if (ImGui::MenuItem("Reset camera")) {
				ResetCamera();
			}
			if (ImGui::MenuItem("Reset layout")) {
				m_ResetLayout = true;
				m_ShowHierarchy = m_ShowInspector = m_ShowStats = m_ShowContent = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			ImGui::MenuItem("ImGui demo window", nullptr, &m_ShowImGuiDemo);
			ImGui::EndMenu();
		}
		if (active && !active->Path.empty()) {
			const std::string path = active->Path.generic_string();
			ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(path.c_str()).x - 16.0f);
			ImGui::TextColored(ToImGui(m_Theme.TextMuted), "%s", path.c_str());
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
			const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26f, nullptr, &center);
			const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.26f, nullptr, &center);
			ImGuiID leftTop = left;
			const ImGuiID leftBottom = ImGui::DockBuilderSplitNode(leftTop, ImGuiDir_Down, 0.22f, nullptr, &leftTop);

			ImGui::DockBuilderDockWindow(kViewportWindow, center);
			ImGui::DockBuilderDockWindow(HierarchyPanel::kWindowName, leftTop);
			ImGui::DockBuilderDockWindow(kStatsWindow, leftBottom);
			ImGui::DockBuilderDockWindow(InspectorPanel::kWindowName, right);
			ImGui::DockBuilderDockWindow(ContentBrowserPanel::kWindowName, bottom);
			ImGui::DockBuilderFinish(dockspace);
		}

		// The panes float on the window: a host window padded by the gap, the dockspace in it (the docking
		// separators are the gaps between panes). Tab bars only where windows share a dock area.
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(m_Theme.Gap, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ToImGui(m_Theme.Window));
		ImGui::Begin("##EditorHost", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove |
		             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);
		const ImVec2 available = ImGui::GetContentRegionAvail();
		ImGui::DockSpace(dockspace, ImVec2(available.x, available.y - m_Theme.Gap), ImGuiDockNodeFlags_AutoHideTabBar);
		ImGui::End();
	}

	void EditorLayer::DrawViewport()
	{
		BeginPane(kViewportWindow, nullptr, m_Theme, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();
		DrawViewportToolbar();

		// The scene fills the rest of the pane, rounded. Rendered at its size in pixels (the framebuffer
		// scale is 2 on Retina displays).
		const ImVec2 size = ImGui::GetContentRegionAvail();
		const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
		m_ViewportSize = { static_cast<uint32_t>(std::max(size.x * scale.x, 0.0f)), static_cast<uint32_t>(std::max(size.y * scale.y, 0.0f)) };

		if (size.x > 0.0f && size.y > 0.0f) {
			const ImVec2 min = ImGui::GetCursorScreenPos();
			const ImVec2 max(min.x + size.x, min.y + size.y);
			const float rounding = m_Theme.ControlRounding + 2.0f;
			ImGui::Dummy(size);
			const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
			ImDrawList* draw = ImGui::GetWindowDrawList();
			ImGuiLayer& imgui = *Application::Get().GetImGuiLayer();
			draw->AddImageRounded(imgui.GetTextureID(*m_ViewportTarget), min, max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, rounding);

			// A mesh from the content browser, dropped into the scene
			if (!IsPlaying()) {
				if (auto mesh = AcceptAssetDrop(AssetKind::Mesh)) {
					const ImVec2 mouse = ImGui::GetMousePos();
					CreateMeshElement(*m_Context, mesh->Asset, {}, DropPosition({ mouse.x, mouse.y }, { min.x, min.y }, { size.x, size.y }));
				}
			}
			if (IsPlaying()) {
				// A frame around the scene, so play mode can't be mistaken for editing
				draw->AddRect(min, max, ToImGuiU32(m_Theme.Mint), rounding, 0, 2.0f);
			} else {
				// The selection outline over the scene, under the gizmo and the tools
				draw->AddImageRounded(imgui.GetTextureID(*m_OutlineTarget), min, max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, rounding);
				const bool onGizmo = DrawGizmo({ min.x, min.y }, { size.x, size.y });
				const bool onTools = DrawToolCapsule({ max.x, min.y });
				// A click on the scene (not on a gizmo or the tools, not Alt for the camera) selects what's under the cursor
				if (clicked && !onGizmo && !onTools && !ImGui::GetIO().KeyAlt) {
					const ImVec2 mouse = ImGui::GetMousePos();
					const glm::uvec2 pixel{
						static_cast<uint32_t>(std::clamp((mouse.x - min.x) * scale.x, 0.0f, std::max(size.x * scale.x - 1.0f, 0.0f))),
						static_cast<uint32_t>(std::clamp((mouse.y - min.y) * scale.y, 0.0f, std::max(size.y * scale.y - 1.0f, 0.0f))) };
					m_Renderer->RequestPick(pixel);
				}
			}
		}
		EndPane();
	}

	bool EditorLayer::DrawToolCapsule(glm::vec2 topRight)
	{
		// A vertical glass capsule in the scene's top-right corner: Move / Rotate / Scale, then the gizmo's
		// axes (World / Local). The selected segment sits in a lighter square, its icon in the accent.
		const float segment = 32.0f, padding = 4.0f, spacing = 2.0f, divider = 7.0f;
		const float width = segment + padding * 2.0f;
		const float height = padding * 2.0f + segment * 5.0f + spacing * 3.0f + divider;
		const ImVec2 min(topRight.x - 10.0f - width, topRight.y + 10.0f), max(min.x + width, min.y + height);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(min, max, ToImGuiU32(m_Theme.Glass), 16.0f);
		draw->AddRect(min, max, ToImGuiU32(m_Theme.GlassEdge), 16.0f);

		struct Tool { const char* Id; Icon Shape; const char* Hint; bool Selected; };
		const Tool tools[] = {
			{ "##move", Icon::Move, "Move (W)", m_GizmoOperation == ImGuizmo::TRANSLATE },
			{ "##rotate", Icon::Rotate, "Rotate (E)", m_GizmoOperation == ImGuizmo::ROTATE },
			{ "##scale", Icon::Scale, "Scale (R)", m_GizmoOperation == ImGuizmo::SCALE },
			{ "##world", Icon::World, "Gizmo axes: World", !m_GizmoLocal },
			{ "##local", Icon::Local, "Gizmo axes: Local (Ctrl snaps)", m_GizmoLocal },
		};
		float y = min.y + padding;
		for (int i = 0; i < 5; i++) {
			if (i == 3) {
				draw->AddLine(ImVec2(min.x + width * 0.5f - 11.0f, y + divider * 0.5f - spacing * 0.5f),
				              ImVec2(min.x + width * 0.5f + 11.0f, y + divider * 0.5f - spacing * 0.5f), ToImGuiU32(m_Theme.GlassEdge), 1.0f);
				y += divider;
			}
			const Tool& tool = tools[i];
			ImGui::SetCursorScreenPos(ImVec2(min.x + padding, y));
			if (IconButton(tool.Id, tool.Shape, segment, tool.Selected ? m_Theme.Accent : m_Theme.TextSecondary, m_Theme, 12.0f,
			               tool.Selected ? &m_Theme.GlassSelected : nullptr, 17.0f)) {
				switch (i) {
					case 0: m_GizmoOperation = ImGuizmo::TRANSLATE; break;
					case 1: m_GizmoOperation = ImGuizmo::ROTATE; break;
					case 2: m_GizmoOperation = ImGuizmo::SCALE; break;
					case 3: m_GizmoLocal = false; break;
					case 4: m_GizmoLocal = true; break;
				}
			}
			ImGui::SetItemTooltip("%s", tool.Hint);
			y += segment + spacing;
		}
		const ImVec2 mouse = ImGui::GetMousePos();
		return mouse.x >= min.x && mouse.x < max.x && mouse.y >= min.y && mouse.y < max.y;
	}

	glm::vec3 EditorLayer::DropPosition(const glm::vec2& mouse, const glm::vec2& viewportMin, const glm::vec2& viewportSize) const
	{
		// The cursor's ray: unproject the cursor at two depths. NDC y points up (as ImGuizmo also assumes
		// for these matrices), screen y down.
		const glm::vec2 uv = (mouse - viewportMin) / glm::max(viewportSize, glm::vec2(1.0f));
		const glm::vec2 ndc{ uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f };
		const glm::mat4 inverse = glm::inverse(m_Camera.GetViewProjection());
		auto unproject = [&](float depth) {
			const glm::vec4 p = inverse * glm::vec4(ndc, depth, 1.0f);
			return glm::vec3(p) / p.w;
		};
		const glm::vec3 origin = unproject(0.0f);
		const glm::vec3 direction = glm::normalize(unproject(1.0f) - origin);

		// Where it meets y = 0, if in front of the camera and not absurdly far (a ray grazing the horizon)
		if (std::abs(direction.y) > 1e-4f) {
			const float t = -origin.y / direction.y;
			if (t > 0.0f && t < 200.0f) {
				return origin + direction * t;
			}
		}
		return m_Camera.GetFocalPoint();
	}

	void EditorLayer::DrawViewportToolbar()
	{
		// The pane's top row: the open steles as tabs, Play / Stop at the right
		const float height = 34.0f;
		const ImVec2 start = ImGui::GetCursorPos();
		DrawSteleTabs(*m_Context, height, m_Theme);

		const float pill = 76.0f;
		const float right = ImGui::GetWindowContentRegionMax().x;
		if (!IsPlaying()) {
			ImGui::SameLine(right - pill);
			ImGui::SetCursorPosY(start.y + (height - 28.0f) * 0.5f);
			ImGui::BeginDisabled(m_Context->Active() == nullptr);
			if (PillButton("play", Icon::Play, "Play", m_Theme.Mint, m_Theme)) {
				Play();
			}
			ImGui::EndDisabled();
		} else {
			if (!m_PlayHasCamera) {
				const char* note = "No primary camera: editor view";
				ImGui::SameLine(right - pill - ImGui::CalcTextSize(note).x - 12.0f);
				ImGui::SetCursorPosY(start.y + (height - ImGui::GetTextLineHeight()) * 0.5f);
				ImGui::TextColored(ToImGui(m_Theme.TextMuted), "%s", note);
			}
			ImGui::SameLine(right - pill);
			ImGui::SetCursorPosY(start.y + (height - 28.0f) * 0.5f);
			if (PillButton("stop", Icon::Stop, "Stop", m_Theme.Danger, m_Theme)) {
				Stop();
			}
		}
		ImGui::SetCursorPos(ImVec2(start.x, start.y + height + 4.0f));
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
		BeginPane(kStatsWindow, &m_ShowStats, m_Theme);
		PaneTitle(kStatsWindow, m_Theme);

		// Label on the left, the value (mono) right-aligned
		auto row = [&](const char* label, const std::string& value, const EditorTheme::Color* color = nullptr) {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
			ImGui::PushFont(nullptr, m_Theme.FontSize - 1.0f);
			ImGui::TextColored(ToImGui(m_Theme.TextMuted), "%s", label);
			ImGui::PopFont();
			ImGui::PushFont(EditorFonts().Mono, m_Theme.FontSize - 1.5f);
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(value.c_str()).x - 6.0f);
			ImGui::TextColored(ToImGui(color ? *color : m_Theme.Text), "%s", value.c_str());
			ImGui::PopFont();
		};

		const ImGuiIO& io = ImGui::GetIO();
		row("Frame", fmt::format("{:.2f} ms \u00b7 {:.0f} fps", 1000.0f / std::max(io.Framerate, 1e-3f), io.Framerate));
		row("Viewport", fmt::format("{} \u00d7 {}", m_ViewportSize.x, m_ViewportSize.y));
		const RendererStats stats = m_Renderer->GetStats();
		row("Draw calls", std::to_string(stats.DrawCalls));
		row("Triangles", std::to_string(stats.Triangles));
		if (OpenStele* stele = m_Context->Active()) {
			const SteleSnapshot snapshot = stele->Source->Read();
			row("Elements", std::to_string(snapshot.ElementCount()));
			row("Version", std::to_string(snapshot.Version()));
			if (IsPlaying()) {
				row("Scripts", std::to_string(m_PlayWorld->ScriptCount()));
			}
		}
		if (const size_t loading = m_Assets->PendingCount(); loading > 0) {
			row("Loading", fmt::format("{} asset{}", loading, loading == 1 ? "" : "s"), &m_Theme.Accent);
		} else if (stats.MissingMeshes > 0) {
			row("Missing", fmt::format("{} mesh{}", stats.MissingMeshes, stats.MissingMeshes == 1 ? "" : "es"), &m_Theme.Danger);
		}
		EndPane();
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
				ImGui::TextColored(ToImGui(m_Theme.Danger), "%s", m_DialogError.c_str());
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
				ImGui::TextColored(ToImGui(m_Theme.Danger), "%s", m_DialogError.c_str());
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
