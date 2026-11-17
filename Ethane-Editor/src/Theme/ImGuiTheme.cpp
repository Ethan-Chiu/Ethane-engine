#include "ImGuiTheme.h"

#include <cfloat>

namespace Ethane {

	void ApplyImGuiTheme(const EditorTheme& t)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		using C = EditorTheme;

		// Shapes: flat and tight
		style.WindowRounding = t.Rounding;
		style.ChildRounding = t.Rounding;
		style.FrameRounding = t.Rounding;
		style.PopupRounding = t.Rounding;
		style.GrabRounding = t.Rounding;
		style.TabRounding = t.Rounding;
		style.ScrollbarRounding = t.Rounding;
		style.WindowPadding = { t.Padding, t.Padding };
		style.FramePadding = { t.Padding, t.Spacing * 0.75f };
		style.ItemSpacing = { t.Padding, t.Spacing };
		style.ItemInnerSpacing = { t.Spacing, t.Spacing };
		style.CellPadding = { t.Spacing, t.Spacing * 0.5f };
		style.IndentSpacing = t.Padding * 2.5f;
		style.ScrollbarSize = 10.0f;
		style.GrabMinSize = 8.0f;
		style.WindowBorderSize = 0.0f;
		style.ChildBorderSize = 0.0f;
		style.FrameBorderSize = 0.0f;
		style.PopupBorderSize = 1.0f;
		style.TabBorderSize = 0.0f;
		style.TabBarBorderSize = 0.0f;
		style.TabBarOverlineSize = 2.0f;
		style.SeparatorTextBorderSize = 1.0f;
		style.DockingSeparatorSize = 1.0f;
		style.WindowMenuButtonPosition = ImGuiDir_None; // no collapse arrow on docked windows
		style.DockingNodeHasCloseButton = false;        // each tab has its own
		style.TabCloseButtonMinWidthSelected = 0.0f;    // close buttons show on hover
		style.TabCloseButtonMinWidthUnselected = FLT_MAX;

		ImVec4* colors = style.Colors;
		auto set = [&](ImGuiCol slot, const C::Color& color) { colors[slot] = ToImGui(color); };
		const C::Color none = C::WithAlpha(t.Background, 0.0f);

		set(ImGuiCol_Text, t.Text);
		set(ImGuiCol_TextDisabled, t.TextMuted);
		set(ImGuiCol_TextLink, t.AccentText);
		set(ImGuiCol_TextSelectedBg, C::WithAlpha(t.Accent, 0.35f));
		set(ImGuiCol_InputTextCursor, t.Text);

		set(ImGuiCol_WindowBg, t.Background);
		set(ImGuiCol_ChildBg, none);
		set(ImGuiCol_PopupBg, t.Background);
		set(ImGuiCol_MenuBarBg, t.Panel);
		set(ImGuiCol_Border, t.Border);
		set(ImGuiCol_BorderShadow, none);
		set(ImGuiCol_ModalWindowDimBg, C::WithAlpha(t.Text, 0.25f));

		set(ImGuiCol_FrameBg, t.Surface);
		set(ImGuiCol_FrameBgHovered, t.SurfaceHover);
		set(ImGuiCol_FrameBgActive, t.SurfaceActive);
		set(ImGuiCol_Button, t.Surface);
		set(ImGuiCol_ButtonHovered, t.SurfaceHover);
		set(ImGuiCol_ButtonActive, t.SurfaceActive);

		// Selected and hovered rows, collapsing headers, menu items
		set(ImGuiCol_Header, t.Highlight);
		set(ImGuiCol_HeaderHovered, t.HighlightHover);
		set(ImGuiCol_HeaderActive, t.Highlight);

		set(ImGuiCol_CheckMark, t.AccentText);
		set(ImGuiCol_CheckboxSelectedBg, t.Surface);
		set(ImGuiCol_SliderGrab, t.Accent);
		set(ImGuiCol_SliderGrabActive, t.AccentText);

		set(ImGuiCol_TitleBg, t.Panel);
		set(ImGuiCol_TitleBgActive, t.Panel);
		set(ImGuiCol_TitleBgCollapsed, t.Panel);

		// Window tabs (shown only where windows share a dock area): the selected one is the page itself
		set(ImGuiCol_Tab, t.Panel);
		set(ImGuiCol_TabHovered, t.Background);
		set(ImGuiCol_TabSelected, t.Background);
		set(ImGuiCol_TabSelectedOverline, t.Accent);
		set(ImGuiCol_TabDimmed, t.Panel);
		set(ImGuiCol_TabDimmedSelected, t.Background);
		set(ImGuiCol_TabDimmedSelectedOverline, none);

		set(ImGuiCol_ScrollbarBg, none);
		set(ImGuiCol_ScrollbarGrab, t.Border);
		set(ImGuiCol_ScrollbarGrabHovered, t.SurfaceActive);
		set(ImGuiCol_ScrollbarGrabActive, t.Accent);

		set(ImGuiCol_Separator, t.Border);
		set(ImGuiCol_SeparatorHovered, t.Accent);
		set(ImGuiCol_SeparatorActive, t.Accent);
		set(ImGuiCol_ResizeGrip, none);
		set(ImGuiCol_ResizeGripHovered, C::WithAlpha(t.Accent, 0.5f));
		set(ImGuiCol_ResizeGripActive, t.Accent);

		set(ImGuiCol_DockingPreview, C::WithAlpha(t.Accent, 0.4f));
		set(ImGuiCol_DockingEmptyBg, t.Background);

		set(ImGuiCol_TableHeaderBg, t.Panel);
		set(ImGuiCol_TableBorderStrong, t.Border);
		set(ImGuiCol_TableBorderLight, t.Border);
		set(ImGuiCol_TableRowBg, none);
		set(ImGuiCol_TableRowBgAlt, C::WithAlpha(t.Panel, 0.5f));
		set(ImGuiCol_TreeLines, t.Border);

		set(ImGuiCol_PlotLines, t.Secondary);
		set(ImGuiCol_PlotLinesHovered, t.AccentText);
		set(ImGuiCol_PlotHistogram, t.Accent);
		set(ImGuiCol_PlotHistogramHovered, t.AccentText);

		set(ImGuiCol_DragDropTarget, t.Accent);
		set(ImGuiCol_DragDropTargetBg, C::WithAlpha(t.Accent, 0.12f));
		set(ImGuiCol_UnsavedMarker, t.Danger);
		set(ImGuiCol_NavCursor, t.Accent);
		set(ImGuiCol_NavWindowingHighlight, t.Accent);
		set(ImGuiCol_NavWindowingDimBg, C::WithAlpha(t.Text, 0.2f));
	}

}
