#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Ethane {

	// The editor's colors by role, and its shapes. Independent of the UI library: panels ask for roles
	// (Selected, Accent, Danger, ...), and an adapter (ImGuiTheme.h) maps them onto the library's style. A
	// future editor UI system would take the same theme. The design lives in docs/ui-mockup.
	struct EditorTheme
	{
		// RGBA, 0..1 (sRGB, as UI colors are usually given)
		using Color = glm::vec4;

		// Areas, outermost first: panes float on the window, cards sit in panes, fields in cards
		Color Window;         // behind the panes, the menu bar
		Color Pane;
		Color Card;           // grouped content inside a pane (inspector components)
		Color Field;          // inputs, buttons
		Color FieldHover;
		Color FieldActive;
		Color Line;           // dividers

		Color Text;           // primary (cream)
		Color TextSecondary;  // values and labels that still matter
		Color TextMuted;      // field labels, pane titles, inactive tabs
		Color TextFaint;      // ids, hints

		Color Hover;          // a row or tile under the cursor (a faint wash)
		Color Selected;       // a selected row or tile (mint, faint)

		// Palette
		Color Accent;         // periwinkle: the active tool, slider fill, toggles, the selection outline
		Color Mint;           // play, folders, the play-mode frame
		Color Danger;         // coral: stop, unsaved, errors
		Color AxisX, AxisY, AxisZ;

		// Floating controls over the viewport (glass)
		Color Glass;
		Color GlassEdge;
		Color GlassSelected;

		// Shapes, in points
		float PaneRounding = 10.0f;
		float CardRounding = 10.0f;
		float ControlRounding = 6.0f;
		float Gap = 6.0f;          // between panes
		float PanePadding = 8.0f;  // inside panes
		float RowHeight = 26.0f;   // tree rows, field rows
		float FontSize = 13.0f;

		// Ethane's palette: cream #FFF3E3, mint #D5E8E6, periwinkle #9FA0FF, coral #FF7171, on charcoal
		static EditorTheme Dark();
		static EditorTheme Default() { return Dark(); }

		// 0xRRGGBB -> Color
		static constexpr Color Hex(uint32_t rgb, float alpha = 1.0f)
		{
			return { float((rgb >> 16) & 0xFF) / 255.0f, float((rgb >> 8) & 0xFF) / 255.0f, float(rgb & 0xFF) / 255.0f, alpha };
		}
		static constexpr Color WithAlpha(Color color, float alpha) { return { color.r, color.g, color.b, alpha }; }
	};

}
