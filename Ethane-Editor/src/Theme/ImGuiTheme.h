#pragma once

#include "Theme/EditorTheme.h"

#include <imgui.h>

namespace Ethane {

	// The ImGui side of the editor's theme: the only place that knows how roles map onto ImGui's style
	void ApplyImGuiTheme(const EditorTheme& theme);

	inline ImVec4 ToImGui(const EditorTheme::Color& color) { return { color.r, color.g, color.b, color.a }; }
	inline ImU32 ToImGuiU32(const EditorTheme::Color& color) { return ImGui::ColorConvertFloat4ToU32(ToImGui(color)); }

}
