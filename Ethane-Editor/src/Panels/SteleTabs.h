#pragma once

#include "Theme/EditorTheme.h"

namespace Ethane {

	class EditorContext;

	// The open steles as flat tabs on one line, `height` tall: click to make one active, the cross (shown on
	// the active and the hovered tab) or a middle click to close it. A dot marks unsaved changes.
	void DrawSteleTabs(EditorContext& context, float height, const EditorTheme& theme);

}
