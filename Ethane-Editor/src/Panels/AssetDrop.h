#pragma once

#include "ContentBrowser/AssetDrag.h"

#include <imgui.h>

#include <optional>

namespace Ethane {

	// Makes the last ImGui item a drop target for assets of `kind` dragged from the content browser. Only
	// while such a drag is in progress, so the item doesn't light up for things it can't take (and it never
	// competes with another target on the same item, like the hierarchy's element drags).
	inline std::optional<AssetDrag> AcceptAssetDrop(AssetKind kind, ImGuiDragDropFlags flags = 0)
	{
		const ImGuiPayload* active = ImGui::GetDragDropPayload();
		if (!active || !active->IsDataType(kAssetDragType))
			return std::nullopt;
		const AssetDrag& dragged = *static_cast<const AssetDrag*>(active->Data);
		if (kind != AssetKind::Any && dragged.Kind != kind)
			return std::nullopt;

		std::optional<AssetDrag> dropped;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetDragType, flags))
				dropped = *static_cast<const AssetDrag*>(payload->Data);
			ImGui::EndDragDropTarget();
		}
		return dropped;
	}

}
