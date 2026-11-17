#include "ImGuiWidgets.h"

#include "Theme/ImGuiTheme.h"

namespace Ethane {

	bool FlatButton(const char* label, bool active, float height, const EditorTheme& theme)
	{
		const ImVec2 text = ImGui::CalcTextSize(label);
		const float padding = ImGui::GetStyle().FramePadding.x;
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const ImVec2 size(text.x + padding * 2.0f, height);
		const bool clicked = ImGui::InvisibleButton(label, size);
		const bool hovered = ImGui::IsItemHovered();

		ImDrawList* draw = ImGui::GetWindowDrawList();
		const EditorTheme::Color color = active || hovered ? theme.Text : theme.TextMuted;
		draw->AddText(ImVec2(min.x + padding, min.y + (height - text.y) * 0.5f), ToImGuiU32(color), label);
		if (active)
			draw->AddRectFilled(ImVec2(min.x + padding, min.y + height - 2.0f), ImVec2(min.x + size.x - padding, min.y + height), ToImGuiU32(theme.AccentText));
		return clicked;
	}

	bool IconButton(const char* id, IconShape shape, const EditorTheme::Color& color, float size, const EditorTheme& theme)
	{
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
		ImDrawList* draw = ImGui::GetWindowDrawList();
		if (ImGui::IsItemHovered())
			draw->AddRectFilled(min, ImVec2(min.x + size, min.y + size), ToImGuiU32(theme.SurfaceHover), theme.Rounding);

		const ImU32 fill = ToImGuiU32(ImGui::GetItemFlags() & ImGuiItemFlags_Disabled ? EditorTheme::WithAlpha(color, 0.4f) : color);
		const float c = size * 0.5f, r = size * 0.24f;
		switch (shape)
		{
			case IconShape::Play:
				draw->AddTriangleFilled(ImVec2(min.x + c - r * 0.8f, min.y + c - r), ImVec2(min.x + c + r, min.y + c),
				                        ImVec2(min.x + c - r * 0.8f, min.y + c + r), fill);
				break;
			case IconShape::Stop:
				draw->AddRectFilled(ImVec2(min.x + c - r * 0.85f, min.y + c - r * 0.85f), ImVec2(min.x + c + r * 0.85f, min.y + c + r * 0.85f), fill);
				break;
		}
		return clicked;
	}

}
