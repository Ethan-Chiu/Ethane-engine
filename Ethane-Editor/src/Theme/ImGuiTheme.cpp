#include "ImGuiTheme.h"

namespace Ethane {

	void ApplyImGuiTheme(const EditorTheme& t)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		using C = EditorTheme;

		// Shapes
		style.WindowRounding = t.WindowRounding;
		style.ChildRounding = t.Rounding;
		style.FrameRounding = t.Rounding;
		style.PopupRounding = t.Rounding;
		style.GrabRounding = t.Rounding;
		style.TabRounding = t.Rounding;
		style.ScrollbarRounding = t.Rounding * 2.0f;
		style.WindowPadding = { t.Padding, t.Padding };
		style.FramePadding = { t.Padding, t.Padding * 0.5f };
		style.ItemSpacing = { t.Spacing, t.Spacing };
		style.ItemInnerSpacing = { t.Spacing, t.Spacing * 0.75f };
		style.IndentSpacing = t.Padding * 2.0f;
		style.ScrollbarSize = 12.0f;
		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.PopupBorderSize = 1.0f;
		style.TabBorderSize = 0.0f;
		style.WindowMenuButtonPosition = ImGuiDir_None; // no collapse arrow on docked windows
		style.DockingNodeHasCloseButton = false;        // each tab has its own
		style.TabCloseButtonMinWidthSelected = 0.0f;    // close buttons show on hover
		style.TabCloseButtonMinWidthUnselected = FLT_MAX;
		style.DockingSeparatorSize = 2.0f;

		ImVec4* colors = style.Colors;
		auto set = [&](ImGuiCol slot, const C::Color& color) { colors[slot] = ToImGui(color); };
		auto accent = [&](float alpha) { return C::WithAlpha(t.Accent, alpha); };

		set(ImGuiCol_Text, t.Text);
		set(ImGuiCol_TextDisabled, t.TextMuted);
		set(ImGuiCol_TextLink, t.Accent);
		set(ImGuiCol_TextSelectedBg, accent(0.35f));
		set(ImGuiCol_InputTextCursor, t.Text);

		set(ImGuiCol_WindowBg, t.Background);
		set(ImGuiCol_ChildBg, C::WithAlpha(t.Background, 0.0f));
		set(ImGuiCol_PopupBg, t.Panel);
		set(ImGuiCol_MenuBarBg, t.Panel);
		set(ImGuiCol_Border, t.Border);
		set(ImGuiCol_BorderShadow, C::WithAlpha(t.Background, 0.0f));
		set(ImGuiCol_ModalWindowDimBg, C::WithAlpha(t.Background, 0.6f));

		set(ImGuiCol_FrameBg, t.Surface);
		set(ImGuiCol_FrameBgHovered, t.SurfaceHover);
		set(ImGuiCol_FrameBgActive, t.SurfaceActive);

		set(ImGuiCol_Button, t.Surface);
		set(ImGuiCol_ButtonHovered, t.SurfaceHover);
		set(ImGuiCol_ButtonActive, t.SurfaceActive);

		// Selected / hovered rows (trees, selectables, collapsing headers)
		set(ImGuiCol_Header, accent(0.22f));
		set(ImGuiCol_HeaderHovered, accent(0.32f));
		set(ImGuiCol_HeaderActive, accent(0.45f));

		set(ImGuiCol_CheckMark, t.Accent);
		set(ImGuiCol_CheckboxSelectedBg, accent(0.25f));
		set(ImGuiCol_SliderGrab, t.Accent);
		set(ImGuiCol_SliderGrabActive, t.AccentActive);

		set(ImGuiCol_TitleBg, t.Background);
		set(ImGuiCol_TitleBgActive, t.Panel);
		set(ImGuiCol_TitleBgCollapsed, t.Background);

		set(ImGuiCol_Tab, t.Background);
		set(ImGuiCol_TabHovered, t.SurfaceHover);
		set(ImGuiCol_TabSelected, t.Panel);
		set(ImGuiCol_TabSelectedOverline, t.Accent);
		set(ImGuiCol_TabDimmed, t.Background);
		set(ImGuiCol_TabDimmedSelected, t.Panel);
		set(ImGuiCol_TabDimmedSelectedOverline, C::WithAlpha(t.Accent, 0.0f));

		set(ImGuiCol_ScrollbarBg, C::WithAlpha(t.Background, 0.0f));
		set(ImGuiCol_ScrollbarGrab, t.Surface);
		set(ImGuiCol_ScrollbarGrabHovered, t.SurfaceHover);
		set(ImGuiCol_ScrollbarGrabActive, t.SurfaceActive);

		set(ImGuiCol_Separator, t.Border);
		set(ImGuiCol_SeparatorHovered, accent(0.6f));
		set(ImGuiCol_SeparatorActive, t.Accent);
		set(ImGuiCol_ResizeGrip, C::WithAlpha(t.Accent, 0.0f));
		set(ImGuiCol_ResizeGripHovered, accent(0.5f));
		set(ImGuiCol_ResizeGripActive, t.Accent);

		set(ImGuiCol_DockingPreview, accent(0.5f));
		set(ImGuiCol_DockingEmptyBg, t.Background);

		set(ImGuiCol_TableHeaderBg, t.Panel);
		set(ImGuiCol_TableBorderStrong, t.Border);
		set(ImGuiCol_TableBorderLight, C::WithAlpha(t.Border, 0.5f));
		set(ImGuiCol_TableRowBg, C::WithAlpha(t.Background, 0.0f));
		set(ImGuiCol_TableRowBgAlt, C::WithAlpha(t.Surface, 0.3f));
		set(ImGuiCol_TreeLines, t.Border);

		set(ImGuiCol_PlotLines, t.Secondary);
		set(ImGuiCol_PlotLinesHovered, t.Accent);
		set(ImGuiCol_PlotHistogram, t.Accent);
		set(ImGuiCol_PlotHistogramHovered, t.AccentHover);

		set(ImGuiCol_DragDropTarget, t.Accent);
		set(ImGuiCol_DragDropTargetBg, accent(0.12f));
		set(ImGuiCol_UnsavedMarker, t.Danger);
		set(ImGuiCol_NavCursor, t.Accent);
		set(ImGuiCol_NavWindowingHighlight, accent(0.7f));
		set(ImGuiCol_NavWindowingDimBg, C::WithAlpha(t.Background, 0.5f));
	}

}
