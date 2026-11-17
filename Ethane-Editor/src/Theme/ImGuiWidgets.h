#pragma once

#include "Theme/EditorTheme.h"

#include <imgui.h>

namespace Ethane {

	// Small flat widgets in the editor's minimal style, drawn with ImGui. Each takes the theme for its
	// colors, so they follow the theme's roles rather than ImGui's style.

	// A text-only button `height` tall: muted, the text color when hovered, and underlined in the accent
	// when `active`. Returns true when clicked.
	bool FlatButton(const char* label, bool active, float height, const EditorTheme& theme);

	enum class IconShape { Play, Stop };
	// A square button with a drawn icon (no glyph font needed). Returns true when clicked.
	bool IconButton(const char* id, IconShape shape, const EditorTheme::Color& color, float size, const EditorTheme& theme);

}
