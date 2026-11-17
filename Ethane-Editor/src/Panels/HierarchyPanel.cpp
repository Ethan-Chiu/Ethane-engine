#include "HierarchyPanel.h"

#include "AssetDrop.h"
#include "EditorActions.h"
#include "EditorContext.h"
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

	void HierarchyPanel::Draw(EditorContext& context, bool* open)
	{
		ImGui::Begin(kWindowName, open);

		OpenStele* stele = context.Active();
		if (!stele)
		{
			ImGui::TextDisabled("No stele open (File > New or Open)");
			ImGui::End();
			return;
		}

		const SteleSnapshot snapshot = stele->Source->Read();
		if (stele->Selected.IsValid() && !snapshot.Contains(stele->Selected))
			stele->Selected = {}; // deleted, or undone away

		for (ElementId id : snapshot.Children())
			DrawElement(context, snapshot, id);

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
		ImGui::End();
	}

	void HierarchyPanel::DrawElement(EditorContext& context, const SteleSnapshot& snapshot, ElementId id)
	{
		OpenStele& stele = *context.Active();
		const SteleElement& element = *snapshot.Find(id);
		const auto children = snapshot.Children(id);

		ImGui::PushID(ImGuiId(id));
		const MeshComponent* mesh = snapshot.Get<MeshComponent>(id);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
		                           ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen |
		                           ImGuiTreeNodeFlags_AllowOverlap; // the visibility toggle sits on the row
		if (children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;
		if (stele.Selected == id)
			flags |= ImGuiTreeNodeFlags_Selected;
		const bool open = ImGui::TreeNodeEx("##node", flags, "%s", element.Name.empty() ? "(unnamed)" : element.Name.c_str());
		const bool rowHovered = ImGui::IsItemHovered();

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

		// Shown/hidden, at the right edge of the row: a small check, faint unless hidden or hovered
		if (mesh)
		{
			const float box = ImGui::GetTextLineHeight() * 0.8f;
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - box - ImGui::GetStyle().FramePadding.x);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(box * 0.1f, box * 0.1f));
			if (mesh->Visible && !rowHovered)
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
			bool visible = mesh->Visible;
			ImGui::SetNextItemWidth(box);
			if (ImGui::Checkbox("##visible", &visible))
			{
				Transaction tx(visible ? "Show " + element.Name : "Hide " + element.Name);
				tx.SetField<MeshComponent>(id, "Visible", visible);
				context.Commit(std::move(tx));
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(mesh->Visible ? "Visible (click to hide)" : "Hidden (click to show)");
			if (mesh->Visible && !rowHovered)
				ImGui::PopStyleVar();
			ImGui::PopStyleVar();
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
