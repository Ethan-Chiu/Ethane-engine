#include "HierarchyPanel.h"

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
		DrawTabs(context);

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

	void HierarchyPanel::DrawTabs(EditorContext& context)
	{
		if (!ImGui::BeginTabBar("##steles", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll))
			return;
		std::optional<size_t> close;
		// When the active stele changed elsewhere (New, Open, Close), select its tab instead of letting the
		// tab bar's current selection switch it back
		const OpenStele* wanted = context.Active();
		const bool changedElsewhere = wanted != m_ShownActive;
		const auto steles = context.Steles();
		for (size_t i = 0; i < steles.size(); i++)
		{
			const OpenStele& stele = *steles[i];
			bool keepOpen = true;
			// The ### part keeps the tab's identity when the label changes (dirty mark, renames on save)
			const std::string label = stele.Name + (stele.IsDirty() ? " *" : "") + "###" + std::to_string(reinterpret_cast<uintptr_t>(&stele));
			const ImGuiTabItemFlags flags = changedElsewhere && &stele == wanted ? ImGuiTabItemFlags_SetSelected : 0;
			if (ImGui::BeginTabItem(label.c_str(), &keepOpen, flags))
			{
				if (!changedElsewhere)
					context.SetActive(i);
				ImGui::EndTabItem();
			}
			if (ImGui::IsItemHovered() && !stele.Path.empty())
				ImGui::SetTooltip("%s", stele.Path.generic_string().c_str());
			if (!keepOpen)
				close = i;
		}
		ImGui::EndTabBar();
		if (close)
			context.Close(*close);
		m_ShownActive = context.Active();
	}

	void HierarchyPanel::DrawElement(EditorContext& context, const SteleSnapshot& snapshot, ElementId id)
	{
		OpenStele& stele = *context.Active();
		const SteleElement& element = *snapshot.Find(id);
		const auto children = snapshot.Children(id);

		ImGui::PushID(ImGuiId(id));
		if (const MeshComponent* mesh = snapshot.Get<MeshComponent>(id))
		{
			bool visible = mesh->Visible;
			if (ImGui::Checkbox("##visible", &visible))
			{
				Transaction tx(visible ? "Show " + element.Name : "Hide " + element.Name);
				tx.SetField<MeshComponent>(id, "Visible", visible);
				context.Commit(std::move(tx));
			}
			ImGui::SameLine();
		}

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
		                           ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
		if (children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;
		if (stele.Selected == id)
			flags |= ImGuiTreeNodeFlags_Selected;
		const bool open = ImGui::TreeNodeEx("##node", flags, "%s", element.Name.empty() ? "(unnamed)" : element.Name.c_str());

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
