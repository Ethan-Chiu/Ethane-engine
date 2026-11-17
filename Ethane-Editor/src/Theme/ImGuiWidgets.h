#pragma once

#include "Theme/EditorTheme.h"

#include <imgui.h>

namespace Ethane {

	// The editor's own widgets, drawn with ImGui in the style of docs/ui-mockup. Each takes the theme and
	// uses its roles, never ImGui's style colors.

	// --- Panes -----------------------------------------------------------------------------------------

	// A dockable window drawn as a rounded pane (docked windows themselves have no background). Call
	// EndPane() whatever it returns, like ImGui::Begin / End.
	bool BeginPane(const char* name, bool* open, const EditorTheme& theme, ImGuiWindowFlags flags = 0);
	void EndPane();

	// A pane's title row: small upper-case label; returns where the row ends (for buttons placed after it)
	void PaneTitle(const char* label, const EditorTheme& theme);

	// A rounded card behind everything drawn between BeginCard and EndCard (the card's height is known only
	// at the end, so its background goes into a channel under the content). Cards don't nest.
	void BeginCard();
	void EndCard(const EditorTheme& theme);

	// --- Icons -----------------------------------------------------------------------------------------

	enum class Icon
	{
		Play, Stop, Move, Rotate, Scale, World, Local, Eye, EyeOff, Plus, ChevronDown, ChevronRight,
		ChevronLeft, More, Search, Close
	};

	// `icon` drawn as strokes / fills in a `size` square centered at `center`
	void DrawIcon(ImDrawList* draw, Icon icon, ImVec2 center, float size, ImU32 color);

	// A `size` square button showing `icon`: a rounded wash when hovered, `selectedBg` behind it when
	// selected. Returns true when clicked.
	bool IconButton(const char* id, Icon icon, float size, const EditorTheme::Color& color, const EditorTheme& theme,
	                float rounding = -1.0f, const EditorTheme::Color* selectedBg = nullptr, float iconSize = 0.0f);

	// --- Controls --------------------------------------------------------------------------------------

	// An on / off switch. Returns true when flipped.
	bool Toggle(const char* id, bool* value, const EditorTheme& theme);

	// A rounded pill with an icon and a label, tinted `tint` (Play / Stop). Returns true when clicked.
	bool PillButton(const char* id, Icon icon, const char* label, const EditorTheme::Color& tint, const EditorTheme& theme);

	// A row of text segments in a rounded capsule, the selected one in a lighter segment with accent text
	// (the tool capsule's style, horizontal). `height` tall; returns the index clicked, or -1.
	int SegmentedControl(const char* id, const char* const* labels, int count, int selected, float height, const EditorTheme& theme);

	// Small text in the mono font (numbers, ids)
	void MonoText(const EditorTheme::Color& color, const char* fmt, ...);

}
