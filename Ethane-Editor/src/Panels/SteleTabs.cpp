#include "SteleTabs.h"

#include "EditorContext.h"
#include "Theme/ImGuiTheme.h"

#include <imgui.h>

#include <optional>

namespace Ethane {

	void DrawSteleTabs(EditorContext& context, float height, const EditorTheme& theme)
	{
		const auto steles = context.Steles();
		const OpenStele* active = context.Active();
		const float padding = ImGui::GetStyle().FramePadding.x * 1.5f;
		const float mark = ImGui::GetFontSize() * 0.6f; // the dot / cross area after the name
		ImDrawList* draw = ImGui::GetWindowDrawList();

		std::optional<size_t> activate, close;
		for (size_t i = 0; i < steles.size(); i++)
		{
			const OpenStele& stele = *steles[i];
			const bool isActive = &stele == active;
			if (i > 0)
				ImGui::SameLine(0.0f, 0.0f);

			ImGui::PushID(static_cast<int>(i));
			const ImVec2 text = ImGui::CalcTextSize(stele.Name.c_str());
			const ImVec2 min = ImGui::GetCursorScreenPos();
			const ImVec2 size(padding + text.x + padding * 0.75f + mark + padding, height);
			ImGui::InvisibleButton("##tab", size);
			const bool hovered = ImGui::IsItemHovered();
			const ImVec2 max(min.x + size.x, min.y + size.y);

			// The cross's area, at the right end
			const ImVec2 markCenter(max.x - padding - mark * 0.5f, min.y + height * 0.5f);
			const ImVec2 mouse = ImGui::GetMousePos();
			const bool overMark = hovered && std::abs(mouse.x - markCenter.x) <= mark * 0.75f && std::abs(mouse.y - markCenter.y) <= mark * 0.75f;

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				(overMark ? close : activate) = i;
			if (ImGui::IsItemClicked(ImGuiMouseButton_Middle))
				close = i;
			if (hovered && !overMark && !stele.Path.empty())
				ImGui::SetTooltip("%s", stele.Path.generic_string().c_str());

			// The active tab is the page (Background) with the accent under it; others sit on the bar
			if (isActive)
			{
				draw->AddRectFilled(min, max, ToImGuiU32(theme.Background));
				draw->AddRectFilled(ImVec2(min.x, max.y - 2.0f), max, ToImGuiU32(theme.AccentText));
			}
			draw->AddText(ImVec2(min.x + padding, min.y + (height - text.y) * 0.5f),
			              ToImGuiU32(isActive || hovered ? theme.Text : theme.TextMuted), stele.Name.c_str());

			if (isActive || hovered)
			{
				const float r = mark * 0.32f;
				const ImU32 color = ToImGuiU32(overMark ? theme.Danger : theme.TextMuted);
				draw->AddLine(ImVec2(markCenter.x - r, markCenter.y - r), ImVec2(markCenter.x + r, markCenter.y + r), color, 1.5f);
				draw->AddLine(ImVec2(markCenter.x - r, markCenter.y + r), ImVec2(markCenter.x + r, markCenter.y - r), color, 1.5f);
			}
			else if (stele.IsDirty())
			{
				draw->AddCircleFilled(markCenter, mark * 0.22f, ToImGuiU32(theme.Danger));
			}
			// Unsaved and showing the cross: the dot moves before the name's end
			if ((isActive || hovered) && stele.IsDirty())
				draw->AddCircleFilled(ImVec2(min.x + padding * 0.5f, min.y + height * 0.5f), mark * 0.18f, ToImGuiU32(theme.Danger));
			ImGui::PopID();
		}

		if (close)
			context.Close(*close);
		else if (activate)
			context.SetActive(*activate);
	}

}
