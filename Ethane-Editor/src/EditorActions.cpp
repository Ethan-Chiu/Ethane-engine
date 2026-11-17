#include "EditorActions.h"

#include "EditorContext.h"
#include "Ethane/Asset/AssetRegistry.h"
#include "Ethane/World/Components.h"

namespace Ethane {

	ElementId CreateMeshElement(EditorContext& context, AssetId mesh, ElementId parent, const glm::vec3& position)
	{
		OpenStele* stele = context.Active();
		if (!stele)
			return {};
		const auto path = context.Registry().GetPath(mesh);
		const std::string name = path && !path->stem().empty() ? path->stem().string() : std::string("Mesh");
		Transaction tx("Add " + name);
		const ElementId id = tx.CreateElement(name, parent);
		tx.AddComponent(id, TransformComponent{ .Position = position });
		tx.AddComponent(id, MeshComponent{ .Mesh = mesh });
		if (!context.Commit(std::move(tx)))
			return {};
		stele->Selected = id;
		return id;
	}

}
