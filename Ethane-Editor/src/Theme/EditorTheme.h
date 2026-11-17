#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Ethane {

	// The editor's colors by role, and its shapes. Independent of the UI library: panels ask for roles
	// (Highlight, Accent, Danger, ...), and an adapter (ImGuiTheme.h) maps them onto the library's style. A
	// future editor UI system would take the same theme.
	struct EditorTheme
	{
		// RGBA, 0..1 (sRGB, as UI colors are usually given)
		using Color = glm::vec4;

		const char* Name = "";

		// Areas, from the page outward
		Color Background;     // window backgrounds, the dockspace
		Color Panel;          // bars: menu bar, the viewport bar, inactive tabs
		Color Surface;        // controls: fields, buttons
		Color SurfaceHover;
		Color SurfaceActive;
		Color Border;         // separators, outlines of drawn shapes

		Color Text;
		Color TextMuted;      // secondary text, hints, inactive tabs and tools

		// Palette roles
		Color Highlight;      // selected rows and headers (mint)
		Color HighlightHover;
		Color Accent;         // small marks: checks, slider grabs, the active underline (periwinkle)
		Color AccentText;     // the accent where it must be read as text or a thin line on Background
		Color Secondary;      // a second mark color: folders, the play-mode frame (mint, deep enough to see)
		Color Danger;         // stopping, errors, unsaved changes (coral)
		Color Selection;      // the selected element's outline in the viewport

		// Shapes, in points. Minimal: square corners, no borders, little padding.
		float Rounding = 0.0f;
		float Padding = 6.0f;   // inside windows
		float Spacing = 4.0f;   // between items

		// Ethane's palette: cream #FFF3E3, mint #D5E8E6, periwinkle #9FA0FF, coral #FF7171
		static EditorTheme Light(); // cream pages, mint selection, dark text
		static EditorTheme Dark();  // neutral greys, cream text, the same marks
		static EditorTheme Default() { return Light(); }

		// 0xRRGGBB -> Color
		static constexpr Color Hex(uint32_t rgb, float alpha = 1.0f)
		{
			return { float((rgb >> 16) & 0xFF) / 255.0f, float((rgb >> 8) & 0xFF) / 255.0f, float(rgb & 0xFF) / 255.0f, alpha };
		}
		static constexpr Color WithAlpha(Color color, float alpha) { return { color.r, color.g, color.b, alpha }; }
		// Relative luminance (approximate, on the sRGB values): which text color reads on `color`
		static float Luminance(const Color& color) { return 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b; }
	};

}
