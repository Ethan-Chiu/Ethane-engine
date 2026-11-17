#include "HierarchyPanel.h"

#include "AssetDrop.h"
#include "EditorActions.h"
#include "EditorContext.h"
#include "Theme/ImGuiTheme.h"
#include "Theme/ImGuiWidgets.h"
#include "Ethane/World/Components.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace Ethane {

	namespace {

		constexpr const char* kDragPayload = "ETHANE_ELEMENT";
		constexpr const char* kRenamePopup = "Rename element";

		int ImGuiId(ElementId id) { return static_cast<int>(id.Value ^ (id.Value >> 32)); }

		// New elements get a transform so they can be placed right away
		void CreateElement(EditorContext& context, ElementId parent)
		{
			Transaction tx("Create element");
			const ElementId id = tx.CreateElement("Element", parent);
			tx.AddComponent(id, TransformComponent{});
			if (context.Commit(std::move(tx)))
				context.Active()->Selected = id;
		}

		void DeleteElement(EditorContext& context, const SteleSnapshot& snapshot, ElementId id)
		{
			Transaction tx("Delete " + snapshot.Find(id)->Name);
			tx.DestroyTree(snapshot, id);
			context.Commit(std::move(tx));
		}

		void MoveElement(EditorContext& context, const SteleSnapshot& snapshot, ElementId id, ElementId parent)
		{
			if (snapshot.Find(id)->Parent == parent)
				return;
			Transaction tx("Move " + snapshot.Find(id)->Name);
			tx.Move(id, parent); // a move into its own subtree is refused by the commit
			context.Commit(std::move(tx));
		}

		std::optional<ElementId> AcceptElementDrop()
		{
			std::optional<ElementId> dropped;
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kDragPayload))
					dropped = *static_cast<const ElementId*>(payload->Data);
				ImGui::EndDragDropTarget();
			}
			return dropped;
		}

	}

	void HierarchyPanel::Draw(EditorContext& context, bool* open, const EditorTheme& theme)
	{
		m_Theme = &theme;
		BeginPane(kWindowName, open, theme);

		// Title row, with "create element" at its right end
		const float titleY = ImGui::GetCursorPosY();
		PaneTitle(kWindowName, theme);
		OpenStele* stele = context.Active();
		if (stele)
		{
			const ImVec2 after = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowContentRegionMax().x - 24.0f, titleY + 2.0f));
			if (IconButton("##create", Icon::Plus, 24.0f, theme.TextMuted, theme))
				CreateElement(context, {});
			ImGui::SetItemTooltip("Create element");
			ImGui::SetCursorPos(after);
		}
		if (!stele)
		{
			ImGui::TextDisabled("No stele open (File > New or Open)");
			EndPane();
			return;
		}

		const SteleSnapshot snapshot = stele->Source->Read();
		if (stele->Selected.IsValid() && !snapshot.Contains(stele->Selected))
			stele->Selected = {}; // deleted, or undone away

		// Rows: ImGui's own highlight off (they get a rounded one), row height from the frame padding
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, (theme.RowHeight - ImGui::GetFontSize()) * 0.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
		m_Rows.Split(ImGui::GetWindowDrawList(), 2);
		m_Rows.SetCurrentChannel(ImGui::GetWindowDrawList(), 1);
		for (ElementId id : snapshot.Children())
			DrawElement(context, snapshot, id);
		m_Rows.Merge(ImGui::GetWindowDrawList());
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		// The rest of the window: drop to move to the top level, click to deselect, right click to create
		ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, std::max(ImGui::GetContentRegionAvail().y, 20.0f)));
		if (auto dropped = AcceptElementDrop())
			MoveElement(context, snapshot, *dropped, {});
		if (auto mesh = AcceptAssetDrop(AssetKind::Mesh))
			CreateMeshElement(context, mesh->Asset);
		if (ImGui::IsItemClicked())
			stele->Selected = {};
		if (ImGui::BeginPopupContextItem("##empty"))
		{
			if (ImGui::MenuItem("Create element"))
				CreateElement(context, {});
			ImGui::EndPopup();
		}

		if (stele->Selected.IsValid() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
		    !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete))
			DeleteElement(context, snapshot, stele->Selected);

		DrawRenamePopup(context, snapshot);
		EndPane();
	}

	void HierarchyPanel::DrawElement(EditorContext& context, const SteleSnapshot& snapshot, ElementId id)
	{
		OpenStele& stele = *context.Active();
		const SteleElement& element = *snapshot.Find(id);
		const auto children = snapshot.Children(id);

		ImGui::PushID(ImGuiId(id));
		const MeshComponent* mesh = snapshot.Get<MeshComponent>(id);
		const EditorTheme& theme = *m_Theme;
		const bool selected = stele.Selected == id;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
		                           ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding |
		                           ImGuiTreeNodeFlags_AllowOverlap; // the visibility toggle sits on the row
		if (children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;
		if (stele.Selected == id)
			flags |= ImGuiTreeNodeFlags_Selected;
		// The label leaves room for the kind marker drawn before the name
		ImGui::PushStyleColor(ImGuiCol_Text, ToImGui(selected ? theme.Text : EditorTheme::WithAlpha(theme.Text, 0.85f)));
		const bool open = ImGui::TreeNodeEx("##node", flags, "    %s", element.Name.empty() ? "(unnamed)" : element.Name.c_str());
		ImGui::PopStyleColor();
		const bool rowHovered = ImGui::IsItemHovered();
		{
			// Row background (rounded, the pane's full width) in the channel under the rows
			ImDrawList* draw = ImGui::GetWindowDrawList();
			const ImVec2 itemMin = ImGui::GetItemRectMin(), itemMax = ImGui::GetItemRectMax();
			const float left = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
			const float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			if (selected || rowHovered)
			{
				m_Rows.SetCurrentChannel(draw, 0);
				draw->AddRectFilled(ImVec2(left, itemMin.y), ImVec2(right, itemMax.y), ToImGuiU32(selected ? theme.Selected : theme.Hover), theme.ControlRounding);
				m_Rows.SetCurrentChannel(draw, 1);
			}

			// Kind marker: meshes periwinkle squares, groups mint squares, cameras and lights dots
			const bool isCamera = snapshot.Get<CameraComponent>(id) != nullptr;
			const bool isLight = snapshot.Get<DirectionalLightComponent>(id) != nullptr;
			EditorTheme::Color color = theme.TextFaint;
			bool round = true;
			if (mesh) { color = theme.Accent; round = false; }
			else if (isCamera) color = theme.TextSecondary;
			else if (isLight) color = theme.Text;
			else if (!children.empty()) { color = theme.Mint; round = false; }
			if (!selected)
				color.a *= 0.75f;
			const ImVec2 center(itemMin.x + ImGui::GetTreeNodeToLabelSpacing() + 5.0f, (itemMin.y + itemMax.y) * 0.5f);
			if (round)
				draw->AddCircleFilled(center, 3.5f, ToImGuiU32(color));
			else
				draw->AddRectFilled(ImVec2(center.x - 3.5f, center.y - 3.5f), ImVec2(center.x + 3.5f, center.y + 3.5f), ToImGuiU32(color), 1.5f);
		}

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			stele.Selected = id;
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(kDragPayload, &id, sizeof(id));
			ImGui::Text("%s", element.Name.c_str());
			ImGui::EndDragDropSource();
		}
		if (auto dropped = AcceptElementDrop(); dropped && *dropped != id)
			MoveElement(context, snapshot, *dropped, id);
		if (auto mesh = AcceptAssetDrop(AssetKind::Mesh))
			CreateMeshElement(context, mesh->Asset, id);
		if (ImGui::BeginPopupContextItem())
		{
			stele.Selected = id;
			if (ImGui::MenuItem("Create child"))
				CreateElement(context, id);
			if (ImGui::MenuItem("Rename"))
			{
				m_Renaming = id;
				m_RenameText = element.Name;
			}
			if (ImGui::MenuItem("Delete", "Del"))
				DeleteElement(context, snapshot, id);
			ImGui::EndPopup();
		}

		// Shown / hidden: an eye at the right end of the row, faint unless hidden or the row is hovered
		if (mesh)
		{
			const float size = theme.RowHeight - 4.0f;
			const float rowTop = ImGui::GetItemRectMin().y;
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - size - 2.0f);
			ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, rowTop + 2.0f));
			const float alpha = !mesh->Visible ? 0.9f : rowHovered ? 0.6f : 0.3f;
			if (IconButton("##visible", mesh->Visible ? Icon::Eye : Icon::EyeOff, size, EditorTheme::WithAlpha(theme.TextSecondary, alpha), theme, -1.0f, nullptr, 14.0f))
			{
				Transaction tx(mesh->Visible ? "Hide " + element.Name : "Show " + element.Name);
				tx.SetField<MeshComponent>(id, "Visible", !mesh->Visible);
				context.Commit(std::move(tx));
			}
			ImGui::SetItemTooltip(mesh->Visible ? "Visible (click to hide)" : "Hidden (click to show)");
		}

		if (open)
		{
			for (ElementId child : children)
				if (snapshot.Contains(child))
					DrawElement(context, snapshot, child);
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void HierarchyPanel::DrawRenamePopup(EditorContext& context, const SteleSnapshot& snapshot)
	{
		if (m_Renaming.IsValid())
		{
			ImGui::OpenPopup(kRenamePopup);
			m_Renaming = {};
			m_RenamingTarget = context.Active()->Selected;
		}
		if (!ImGui::BeginPopup(kRenamePopup))
			return;
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		const bool done = ImGui::InputText("##name", &m_RenameText, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		if (done && snapshot.Contains(m_RenamingTarget))
		{
			Transaction tx("Rename " + snapshot.Find(m_RenamingTarget)->Name);
			tx.Rename(m_RenamingTarget, m_RenameText);
			context.Commit(std::move(tx));
		}
		if (done || ImGui::IsKeyPressed(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

}
