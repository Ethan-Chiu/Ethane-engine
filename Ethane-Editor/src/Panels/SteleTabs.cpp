#include "SteleTabs.h"

#include "EditorContext.h"
#include "Theme/ImGuiTheme.h"
#include "Theme/ImGuiWidgets.h"

#include <imgui.h>

#include <cmath>
#include <optional>

namespace Ethane {

	void DrawSteleTabs(EditorContext& context, float height, const EditorTheme& theme)
	{
		const auto steles = context.Steles();
		const OpenStele* active = context.Active();
		const float tabHeight = height - 10.0f;
		const float closeSize = 10.0f;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const float top = ImGui::GetCursorPosY();

		std::optional<size_t> activate, close;
		for (size_t i = 0; i < steles.size(); i++)
		{
			const OpenStele& stele = *steles[i];
			const bool isActive = &stele == active;
			if (i > 0)
				ImGui::SameLine(0.0f, 2.0f);
			ImGui::SetCursorPosY(top + (height - tabHeight) * 0.5f);

			ImGui::PushID(static_cast<int>(i));
			ImGui::PushFont(isActive ? EditorFonts().Medium : EditorFonts().Regular, 0.0f);
			const ImVec2 text = ImGui::CalcTextSize(stele.Name.c_str());
			const float dot = stele.IsDirty() ? 6.0f + 8.0f : 0.0f;
			const ImVec2 size(12.0f + dot + text.x + 8.0f + closeSize + 10.0f, tabHeight);
			const ImVec2 min = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton("##tab", size);
			const bool hovered = ImGui::IsItemHovered();
			const ImVec2 max(min.x + size.x, min.y + size.y);

			// The cross at the right end: shown on the active and the hovered tab
			const ImVec2 cross(max.x - 10.0f - closeSize * 0.5f, min.y + tabHeight * 0.5f);
			const ImVec2 mouse = ImGui::GetMousePos();
			const bool overCross = hovered && std::abs(mouse.x - cross.x) <= closeSize && std::abs(mouse.y - cross.y) <= closeSize;

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				(overCross ? close : activate) = i;
			if (ImGui::IsItemClicked(ImGuiMouseButton_Middle))
				close = i;
			if (hovered && !overCross && !stele.Path.empty())
				ImGui::SetTooltip("%s", stele.Path.generic_string().c_str());

			if (isActive || hovered)
				draw->AddRectFilled(min, max, ToImGuiU32(EditorTheme::WithAlpha(theme.Text, isActive ? 0.07f : 0.04f)), theme.ControlRounding + 2.0f);
			float x = min.x + 12.0f;
			if (stele.IsDirty())
			{
				draw->AddCircleFilled(ImVec2(x + 3.0f, min.y + tabHeight * 0.5f), 3.0f, ToImGuiU32(theme.Danger));
				x += dot;
			}
			draw->AddText(ImVec2(x, min.y + (tabHeight - text.y) * 0.5f), ToImGuiU32(isActive || hovered ? theme.Text : theme.TextMuted), stele.Name.c_str());
			if (isActive || hovered)
			{
				if (overCross)
					draw->AddCircleFilled(cross, closeSize * 0.8f, ToImGuiU32(EditorTheme::WithAlpha(theme.Text, 0.08f)));
				DrawIcon(draw, Icon::Close, cross, closeSize, ToImGuiU32(overCross ? theme.Text : theme.TextMuted));
			}
			ImGui::PopFont();
			ImGui::PopID();
		}

		if (close)
			context.Close(*close);
		else if (activate)
			context.SetActive(*activate);
	}

}
