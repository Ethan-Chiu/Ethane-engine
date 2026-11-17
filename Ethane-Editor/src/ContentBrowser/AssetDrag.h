#pragma once

#include "Ethane/Schema/Ids.h"

namespace Ethane {

	// An asset being dragged out of the content browser. Panels that accept drops read this; how it travels
	// (ImGui's drag-and-drop payload named kAssetDragType today) is the UI layer's business.
	struct AssetDrag
	{
		AssetId Asset;
		AssetKind Kind = AssetKind::Any;
	};

	inline constexpr const char* kAssetDragType = "ETHANE_ASSET";

}
