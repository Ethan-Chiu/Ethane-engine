#pragma once

#include "Ethane/Stele/SteleSnapshot.h"

#include <string>

namespace Ethane {

	class EditorContext;

	// Tabs for the open steles, and the active stele's elements as a tree:
	// - click selects, drag an element onto another to make it a child (or onto empty space: top level)
	// - right click: create a child, rename, delete; right click on empty space: create an element
	// - Delete removes the selected element and its descendants
	class HierarchyPanel
	{
	public:
		static constexpr const char* kWindowName = "Hierarchy";

		void Draw(EditorContext& context, bool* open);

	private:
		void DrawTabs(EditorContext& context);
		void DrawElement(EditorContext& context, const SteleSnapshot& snapshot, ElementId id);
		void DrawRenamePopup(EditorContext& context, const SteleSnapshot& snapshot);

		ElementId m_Renaming;        // set for one frame: open the rename popup for this element
		ElementId m_RenamingTarget;  // the element the open rename popup applies to
		std::string m_RenameText;
		const void* m_ShownActive = nullptr; // the active stele the tab bar last showed
	};

}
