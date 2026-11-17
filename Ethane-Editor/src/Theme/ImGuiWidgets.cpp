#include "ImGuiWidgets.h"

#include "Theme/ImGuiTheme.h"

#include <cctype>
#include <cmath>
#include <cstdarg>
#include <string>

namespace Ethane {

	namespace {

		ImVec2 Add(ImVec2 a, ImVec2 b) { return { a.x + b.x, a.y + b.y }; }

		// The card being drawn: its channel splitter and where it started
		ImDrawListSplitter g_CardSplitter;
		ImVec2 g_CardMin;

	}

	// --- Panes -----------------------------------------------------------------------------------------

	bool BeginPane(const char* name, bool* open, const EditorTheme& theme, ImGuiWindowFlags flags)
	{
		const bool visible = ImGui::Begin(name, open, flags);
		// Drawn first, so it's under everything the window draws; the window's own background is none
		const ImVec2 min = ImGui::GetWindowPos(), size = ImGui::GetWindowSize();
		ImGui::GetWindowDrawList()->AddRectFilled(min, Add(min, size), ToImGuiU32(theme.Pane), theme.PaneRounding);
		return visible;
	}

	void EndPane() { ImGui::End(); }

	void PaneTitle(const char* label, const EditorTheme& theme)
	{
		std::string upper = label;
		for (char& c : upper)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		const float height = 28.0f;
		const ImVec2 start = ImGui::GetCursorPos();
		ImGui::PushFont(EditorFonts().SemiBold, 10.5f);
		ImGui::SetCursorPos(ImVec2(start.x + 6.0f, start.y + (height - ImGui::GetTextLineHeight()) * 0.5f));
		ImGui::TextColored(ToImGui(theme.TextMuted), "%s", upper.c_str());
		ImGui::PopFont();
		ImGui::SetCursorPos(ImVec2(start.x, start.y + height));
	}

	void BeginCard()
	{
		ImDrawList* draw = ImGui::GetWindowDrawList();
		g_CardSplitter.Split(draw, 2);
		g_CardSplitter.SetCurrentChannel(draw, 1);
		g_CardMin = ImGui::GetCursorScreenPos();
		ImGui::BeginGroup();
	}

	void EndCard(const EditorTheme& theme)
	{
		ImGui::EndGroup();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 max(g_CardMin.x + ImGui::GetContentRegionAvail().x, ImGui::GetItemRectMax().y);
		g_CardSplitter.SetCurrentChannel(draw, 0);
		draw->AddRectFilled(g_CardMin, max, ToImGuiU32(theme.Card), theme.CardRounding);
		g_CardSplitter.Merge(draw);
	}

	// --- Icons -----------------------------------------------------------------------------------------

	void DrawIcon(ImDrawList* draw, Icon icon, ImVec2 c, float size, ImU32 color)
	{
		const float s = size / 18.0f; // drawn on an 18-unit grid, like the mockup's SVGs
		auto p = [&](float x, float y) { return ImVec2(c.x + (x - 9.0f) * s, c.y + (y - 9.0f) * s); };
		const float w = 1.5f * std::max(s, 0.75f);
		auto line = [&](float x0, float y0, float x1, float y1) { draw->AddLine(p(x0, y0), p(x1, y1), color, w); };

		switch (icon)
		{
			case Icon::Play:
				draw->AddTriangleFilled(p(5.0f, 3.5f), p(15.0f, 9.0f), p(5.0f, 14.5f), color);
				break;
			case Icon::Stop:
				draw->AddRectFilled(p(4.5f, 4.5f), p(13.5f, 13.5f), color, 2.0f * s);
				break;
			case Icon::Move:
				line(9, 2, 9, 16); line(2, 9, 16, 9);
				line(9, 2, 6.8f, 4.2f); line(9, 2, 11.2f, 4.2f); line(9, 16, 6.8f, 13.8f); line(9, 16, 11.2f, 13.8f);
				line(2, 9, 4.2f, 6.8f); line(2, 9, 4.2f, 11.2f); line(16, 9, 13.8f, 6.8f); line(16, 9, 13.8f, 11.2f);
				break;
			case Icon::Rotate:
				draw->PathArcTo(p(9, 9), 5.5f * s, 3.14159f * 0.25f, 3.14159f * 1.85f);
				draw->PathStroke(color, 0, w);
				line(14.5f, 2.5f, 14.5f, 6.0f); line(14.5f, 6.0f, 11.0f, 6.0f);
				break;
			case Icon::Scale:
				line(3, 7, 3, 3); line(3, 3, 7, 3); line(3, 3, 8, 8);
				line(15, 11, 15, 15); line(15, 15, 11, 15); line(15, 15, 10, 10);
				line(10, 3, 15, 3); line(15, 3, 15, 8); line(8, 15, 3, 15); line(3, 15, 3, 10);
				break;
			case Icon::World:
				draw->AddCircle(p(9, 9), 6.5f * s, color, 0, w);
				line(2.5f, 9, 15.5f, 9);
				draw->AddEllipse(p(9, 9), ImVec2(2.6f * s, 6.5f * s), color, 0.0f, 0, w);
				break;
			case Icon::Local:
			{
				const ImVec2 hex[] = { p(9, 2.5f), p(15, 5.7f), p(15, 12.3f), p(9, 15.5f), p(3, 12.3f), p(3, 5.7f) };
				draw->AddPolyline(hex, 6, color, ImDrawFlags_Closed, w);
				line(3, 5.7f, 9, 9); line(9, 9, 15, 5.7f); line(9, 9, 9, 15.5f);
				break;
			}
			case Icon::Eye:
			case Icon::EyeOff:
				draw->AddEllipse(p(9, 9), ImVec2(7.0f * s, 4.2f * s), color, 0.0f, 0, w * 0.85f);
				draw->AddCircleFilled(p(9, 9), 2.0f * s, color);
				if (icon == Icon::EyeOff)
					line(3, 15, 15, 3);
				break;
			case Icon::Plus:
				line(9, 3, 9, 15); line(3, 9, 15, 9);
				break;
			case Icon::ChevronDown:
				line(5, 7, 9, 11); line(9, 11, 13, 7);
				break;
			case Icon::ChevronRight:
				line(7, 5, 11, 9); line(11, 9, 7, 13);
				break;
			case Icon::ChevronLeft:
				line(11, 4, 6, 9); line(6, 9, 11, 14);
				break;
			case Icon::More:
				for (float x : { 4.0f, 9.0f, 14.0f })
					draw->AddCircleFilled(p(x, 9), 1.3f * s, color);
				break;
			case Icon::Search:
				draw->AddCircle(p(8, 8), 5.0f * s, color, 0, w);
				line(11.7f, 11.7f, 15.5f, 15.5f);
				break;
			case Icon::Close:
				line(5, 5, 13, 13); line(13, 5, 5, 13);
				break;
		}
	}

	bool IconButton(const char* id, Icon icon, float size, const EditorTheme::Color& color, const EditorTheme& theme,
	                float rounding, const EditorTheme::Color* selectedBg, float iconSize)
	{
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
		const bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 max(min.x + size, min.y + size);
		const float r = rounding < 0.0f ? theme.ControlRounding : rounding;
		if (selectedBg)
			draw->AddRectFilled(min, max, ToImGuiU32(*selectedBg), r);
		else if (ImGui::IsItemHovered() && !disabled)
			draw->AddRectFilled(min, max, ToImGuiU32(EditorTheme::WithAlpha(theme.Text, 0.07f)), r);
		const EditorTheme::Color shown = disabled ? EditorTheme::WithAlpha(color, color.a * 0.4f) : color;
		DrawIcon(draw, icon, ImVec2(min.x + size * 0.5f, min.y + size * 0.5f), iconSize > 0.0f ? iconSize : size * 0.55f, ToImGuiU32(shown));
		return clicked;
	}

	// --- Controls --------------------------------------------------------------------------------------

	bool Toggle(const char* id, bool* value, const EditorTheme& theme)
	{
		const ImVec2 size(30.0f, 18.0f);
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(id, size);
		if (clicked)
			*value = !*value;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const EditorTheme::Color track = *value ? theme.Accent : (ImGui::IsItemHovered() ? theme.FieldActive : theme.FieldHover);
		draw->AddRectFilled(min, ImVec2(min.x + size.x, min.y + size.y), ToImGuiU32(track), size.y * 0.5f);
		const float knob = size.y * 0.5f - 2.0f;
		const float x = *value ? min.x + size.x - 2.0f - knob : min.x + 2.0f + knob;
		draw->AddCircleFilled(ImVec2(x, min.y + size.y * 0.5f), knob, ToImGuiU32(*value ? theme.Text : theme.TextSecondary));
		return clicked;
	}

	bool PillButton(const char* id, Icon icon, const char* label, const EditorTheme::Color& tint, const EditorTheme& theme)
	{
		ImGui::PushFont(EditorFonts().Medium, 0.0f);
		const ImVec2 text = ImGui::CalcTextSize(label);
		const float height = 28.0f, iconSize = 12.0f;
		const ImVec2 size(10.0f + iconSize + 7.0f + text.x + 12.0f, height);
		const ImVec2 min = ImGui::GetCursorScreenPos();
		ImGui::PushID(id);
		const bool clicked = ImGui::InvisibleButton("##pill", size);
		ImGui::PopID();
		const bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
		const float alpha = disabled ? 0.4f : 1.0f;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const float wash = ImGui::IsItemHovered() && !disabled ? 0.16f : 0.09f;
		draw->AddRectFilled(min, ImVec2(min.x + size.x, min.y + size.y), ToImGuiU32(EditorTheme::WithAlpha(tint, wash * alpha)), height * 0.5f);
		const ImU32 color = ToImGuiU32(EditorTheme::WithAlpha(tint, alpha));
		DrawIcon(draw, icon, ImVec2(min.x + 10.0f + iconSize * 0.5f, min.y + height * 0.5f), iconSize, color);
		draw->AddText(ImVec2(min.x + 10.0f + iconSize + 7.0f, min.y + (height - text.y) * 0.5f), color, label);
		ImGui::PopFont();
		(void)theme;
		return clicked;
	}

	int SegmentedControl(const char* id, const char* const* labels, int count, int selected, float height, const EditorTheme& theme)
	{
		const float padding = 3.0f, segmentPadding = 10.0f;
		ImGui::PushID(id);
		ImGui::PushFont(EditorFonts().Medium, theme.FontSize - 1.0f);
		float width = padding * 2.0f;
		for (int i = 0; i < count; i++)
			width += ImGui::CalcTextSize(labels[i]).x + segmentPadding * 2.0f;
		const ImVec2 min = ImGui::GetCursorScreenPos();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(min, ImVec2(min.x + width, min.y + height), ToImGuiU32(theme.Field), height * 0.5f);

		int clicked = -1;
		float x = min.x + padding;
		const float inner = height - padding * 2.0f;
		for (int i = 0; i < count; i++)
		{
			const ImVec2 text = ImGui::CalcTextSize(labels[i]);
			const ImVec2 segMin(x, min.y + padding), segMax(x + text.x + segmentPadding * 2.0f, min.y + padding + inner);
			ImGui::SetCursorScreenPos(segMin);
			ImGui::PushID(i);
			if (ImGui::InvisibleButton("##segment", ImVec2(segMax.x - segMin.x, inner)) && i != selected)
				clicked = i;
			const bool hovered = ImGui::IsItemHovered();
			ImGui::PopID();
			if (i == selected)
				draw->AddRectFilled(segMin, segMax, ToImGuiU32(theme.GlassSelected), inner * 0.5f);
			else if (hovered)
				draw->AddRectFilled(segMin, segMax, ToImGuiU32(theme.Hover), inner * 0.5f);
			const EditorTheme::Color& color = i == selected ? theme.Accent : hovered ? theme.Text : theme.TextMuted;
			draw->AddText(ImVec2(segMin.x + segmentPadding, segMin.y + (inner - text.y) * 0.5f), ToImGuiU32(color), labels[i]);
			x = segMax.x;
		}
		ImGui::PopFont();
		ImGui::PopID();
		// Leave the cursor after the control, on its line
		ImGui::SetCursorScreenPos(min);
		ImGui::Dummy(ImVec2(width, height));
		return clicked;
	}

	void MonoText(const EditorTheme::Color& color, const char* fmt, ...)
	{
		ImGui::PushFont(EditorFonts().Mono, ImGui::GetStyle().FontSizeBase - 1.5f);
		ImGui::PushStyleColor(ImGuiCol_Text, ToImGui(color));
		va_list args;
		va_start(args, fmt);
		ImGui::TextV(fmt, args);
		va_end(args);
		ImGui::PopStyleColor();
		ImGui::PopFont();
	}

}
