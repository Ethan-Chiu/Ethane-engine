#include "ImGuiTheme.h"

#include <cfloat>
#include <filesystem>
#include <string>

namespace Ethane {

	namespace {
		ImGuiFonts g_Fonts;
	}

	void LoadImGuiFonts(const char* directory)
	{
		ImGuiIO& io = ImGui::GetIO();
		const std::string dir = directory;
		auto load = [&](const char* file) -> ImFont* {
			const std::string path = dir + "/" + file;
			if (!std::filesystem::exists(path))
				return nullptr;
			// The size is only a default: ImGui 1.92+ rasterizes each size on demand (PushFont(font, size))
			return io.Fonts->AddFontFromFileTTF(path.c_str(), 13.0f);
		};
		g_Fonts.Regular = load("inter/Inter-Regular.ttf");
		g_Fonts.Medium = load("inter/Inter-Medium.ttf");
		g_Fonts.SemiBold = load("inter/Inter-SemiBold.ttf");
		g_Fonts.Mono = load("jetbrains-mono/JetBrainsMono-Regular.ttf");
		if (g_Fonts.Regular)
			io.FontDefault = g_Fonts.Regular;
		// Missing files: fall back to whatever is loaded, so callers can always push a font
		ImFont* fallback = g_Fonts.Regular ? g_Fonts.Regular : io.FontDefault;
		for (ImFont** font : { &g_Fonts.Regular, &g_Fonts.Medium, &g_Fonts.SemiBold, &g_Fonts.Mono })
			if (!*font)
				*font = fallback;
	}

	const ImGuiFonts& EditorFonts() { return g_Fonts; }

	void ApplyImGuiTheme(const EditorTheme& t)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		using C = EditorTheme;

		style.FontSizeBase = t.FontSize;
		style._NextFrameFontSizeBase = t.FontSize; // takes effect from the next frame (ImGui 1.92+)

		// Shapes. Docked windows have no rounding of their own: panes are drawn by the panels (BeginPane).
		style.WindowRounding = t.PaneRounding;
		style.ChildRounding = t.ControlRounding;
		style.FrameRounding = t.ControlRounding;
		style.PopupRounding = t.ControlRounding + 2.0f;
		style.GrabRounding = t.ControlRounding;
		style.TabRounding = t.ControlRounding + 2.0f;
		style.ScrollbarRounding = 8.0f;
		style.WindowPadding = { t.PanePadding, t.PanePadding };
		style.FramePadding = { 8.0f, 5.0f };
		style.ItemSpacing = { 6.0f, 4.0f };
		style.ItemInnerSpacing = { 6.0f, 4.0f };
		style.CellPadding = { 4.0f, 2.0f };
		style.IndentSpacing = 16.0f;
		style.ScrollbarSize = 8.0f;
		style.GrabMinSize = 2.0f;
		style.WindowBorderSize = 0.0f;
		style.ChildBorderSize = 0.0f;
		style.FrameBorderSize = 0.0f;
		style.PopupBorderSize = 0.0f;
		style.TabBorderSize = 0.0f;
		style.TabBarBorderSize = 0.0f;
		style.TabBarOverlineSize = 0.0f;
		style.SeparatorTextBorderSize = 1.0f;
		style.DockingSeparatorSize = t.Gap;              // the gaps between panes
		style.WindowMenuButtonPosition = ImGuiDir_None;
		style.DockingNodeHasCloseButton = false;
		style.TabCloseButtonMinWidthSelected = 0.0f;
		style.TabCloseButtonMinWidthUnselected = FLT_MAX;
		style.SelectableTextAlign = { 0.0f, 0.5f };

		ImVec4* colors = style.Colors;
		auto set = [&](ImGuiCol slot, const C::Color& color) { colors[slot] = ToImGui(color); };
		const C::Color none = C::WithAlpha(t.Window, 0.0f);

		set(ImGuiCol_Text, t.Text);
		set(ImGuiCol_TextDisabled, t.TextMuted);
		set(ImGuiCol_TextLink, t.Accent);
		set(ImGuiCol_TextSelectedBg, C::WithAlpha(t.Accent, 0.35f));
		set(ImGuiCol_InputTextCursor, t.Text);

		// Docked windows are transparent: the pane behind them is drawn by BeginPane
		set(ImGuiCol_WindowBg, none);
		set(ImGuiCol_ChildBg, none);
		set(ImGuiCol_PopupBg, t.Field);
		set(ImGuiCol_MenuBarBg, t.Window);
		set(ImGuiCol_Border, none); // ImGui draws docking splitters with it: the gaps between panes must stay the window's
		set(ImGuiCol_BorderShadow, none);
		set(ImGuiCol_ModalWindowDimBg, C::WithAlpha(t.Window, 0.6f));

		set(ImGuiCol_FrameBg, t.Field);
		set(ImGuiCol_FrameBgHovered, t.FieldHover);
		set(ImGuiCol_FrameBgActive, t.FieldActive);
		set(ImGuiCol_Button, t.Field);
		set(ImGuiCol_ButtonHovered, t.FieldHover);
		set(ImGuiCol_ButtonActive, t.FieldActive);

		set(ImGuiCol_Header, t.Selected);
		set(ImGuiCol_HeaderHovered, t.Hover);
		set(ImGuiCol_HeaderActive, t.Selected);

		set(ImGuiCol_CheckMark, t.Accent);
		set(ImGuiCol_CheckboxSelectedBg, t.Field);
		set(ImGuiCol_SliderGrab, t.Accent);
		set(ImGuiCol_SliderGrabActive, t.Accent);

		set(ImGuiCol_TitleBg, t.Window);
		set(ImGuiCol_TitleBgActive, t.Window);
		set(ImGuiCol_TitleBgCollapsed, t.Window);

		set(ImGuiCol_Tab, none);
		set(ImGuiCol_TabHovered, t.Hover);
		set(ImGuiCol_TabSelected, t.Pane);
		set(ImGuiCol_TabSelectedOverline, none);
		set(ImGuiCol_TabDimmed, none);
		set(ImGuiCol_TabDimmedSelected, t.Pane);
		set(ImGuiCol_TabDimmedSelectedOverline, none);

		set(ImGuiCol_ScrollbarBg, none);
		set(ImGuiCol_ScrollbarGrab, t.Field);
		set(ImGuiCol_ScrollbarGrabHovered, t.FieldHover);
		set(ImGuiCol_ScrollbarGrabActive, t.FieldActive);

		// The docking gaps show the window; a faint accent while resizing
		set(ImGuiCol_Separator, none);
		set(ImGuiCol_SeparatorHovered, C::WithAlpha(t.Accent, 0.35f));
		set(ImGuiCol_SeparatorActive, C::WithAlpha(t.Accent, 0.6f));
		set(ImGuiCol_ResizeGrip, none);
		set(ImGuiCol_ResizeGripHovered, none);
		set(ImGuiCol_ResizeGripActive, none);

		set(ImGuiCol_DockingPreview, C::WithAlpha(t.Accent, 0.3f));
		set(ImGuiCol_DockingEmptyBg, t.Window);

		set(ImGuiCol_TableHeaderBg, t.Card);
		set(ImGuiCol_TableBorderStrong, t.Line);
		set(ImGuiCol_TableBorderLight, t.Line);
		set(ImGuiCol_TableRowBg, none);
		set(ImGuiCol_TableRowBgAlt, t.Hover);
		set(ImGuiCol_TreeLines, t.Line);

		set(ImGuiCol_PlotLines, t.Mint);
		set(ImGuiCol_PlotLinesHovered, t.Accent);
		set(ImGuiCol_PlotHistogram, t.Accent);
		set(ImGuiCol_PlotHistogramHovered, t.Accent);

		set(ImGuiCol_DragDropTarget, t.Accent);
		set(ImGuiCol_DragDropTargetBg, C::WithAlpha(t.Accent, 0.10f));
		set(ImGuiCol_UnsavedMarker, t.Danger);
		set(ImGuiCol_NavCursor, t.Accent);
		set(ImGuiCol_NavWindowingHighlight, t.Accent);
		set(ImGuiCol_NavWindowingDimBg, C::WithAlpha(t.Window, 0.5f));
	}

}
