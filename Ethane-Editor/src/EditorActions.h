#pragma once

#include "Ethane/Schema/Ids.h"

#include <glm/glm.hpp>

namespace Ethane {

	class EditorContext;

	// Editing operations shared by several panels. Independent of the UI: each is one undoable transaction
	// on the active stele.

	// A new element showing `mesh`, named after its file, with a Transform at `position` (relative to
	// `parent`); it becomes the selection. Null if there's no active stele or the commit was refused.
	ElementId CreateMeshElement(EditorContext& context, AssetId mesh, ElementId parent = {}, const glm::vec3& position = glm::vec3(0.0f));

}
