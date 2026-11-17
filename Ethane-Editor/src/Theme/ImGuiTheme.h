#pragma once

#include "Theme/EditorTheme.h"

#include <imgui.h>

namespace Ethane {

	// The ImGui side of the editor's theme: the only place that knows how roles map onto ImGui's style

	// The editor's fonts (Inter for UI text, JetBrains Mono for numbers and ids). Null until loaded; ImGui's
	// default font stands in.
	struct ImGuiFonts
	{
		ImFont* Regular = nullptr;
		ImFont* Medium = nullptr;
		ImFont* SemiBold = nullptr;
		ImFont* Mono = nullptr;
	};

	// Loads the fonts from `directory` (inter/, jetbrains-mono/) and makes Regular the default
	void LoadImGuiFonts(const char* directory);
	const ImGuiFonts& EditorFonts();

	void ApplyImGuiTheme(const EditorTheme& theme);

	inline ImVec4 ToImGui(const EditorTheme::Color& color) { return { color.r, color.g, color.b, color.a }; }
	inline ImU32 ToImGuiU32(const EditorTheme::Color& color) { return ImGui::ColorConvertFloat4ToU32(ToImGui(color)); }

}
