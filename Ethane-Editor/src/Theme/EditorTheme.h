#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Ethane {

	// The editor's colors by role, and its shapes. Independent of the UI library: panels ask for roles
	// (Accent, Danger, ...), and an adapter (ImGuiTheme.h) maps them onto the library's style. A future
	// editor UI system would take the same theme.
	struct EditorTheme
	{
		// RGBA, 0..1 (sRGB, as UI colors are usually given)
		using Color = glm::vec4;

		// Neutrals, darkest to lightest
		Color Background;    // behind everything: window backgrounds, the dockspace
		Color Panel;         // raised areas: menu bar, popups, inactive tabs
		Color Surface;       // controls: fields, buttons
		Color SurfaceHover;
		Color SurfaceActive;
		Color Border;

		Color Text;
		Color TextMuted;     // secondary text, hints, disabled

		// Palette
		Color Accent;        // the main highlight: active tab, checks, sliders, links
		Color AccentHover;
		Color AccentActive;
		Color Secondary;     // a calmer second highlight: folders, the play-mode frame
		Color Danger;        // destructive or stopping actions
		Color Selection;     // selected things in the viewport (the outline) and lists

		// Shapes, in points
		float WindowRounding = 6.0f;
		float Rounding = 4.0f;      // controls, tabs, popups
		float Spacing = 6.0f;       // between items
		float Padding = 8.0f;       // inside windows

		// Ethane's palette: periwinkle #9FA0FF, mint #D5E8E6, cream #FFF3E3, coral #FF7171, on dark
		// neutrals tinted toward the periwinkle
		static EditorTheme Default();

		// 0xRRGGBB -> Color
		static constexpr Color Hex(uint32_t rgb, float alpha = 1.0f)
		{
			return { float((rgb >> 16) & 0xFF) / 255.0f, float((rgb >> 8) & 0xFF) / 255.0f, float(rgb & 0xFF) / 255.0f, alpha };
		}
		static constexpr Color WithAlpha(Color color, float alpha) { return { color.r, color.g, color.b, alpha }; }
	};

}
