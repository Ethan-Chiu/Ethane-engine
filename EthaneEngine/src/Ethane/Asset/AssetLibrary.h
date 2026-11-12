#pragma once

#include "Ethane/Asset/AssetRegistry.h"
#include "Ethane/Core/Base.h"
#include "Ethane/GfxBackend/GfxObject.h"

#include <unordered_map>
#include <unordered_set>

namespace Ethane {

	class GfxBackendAPI;
	class Mesh;

	// Loads assets by id (through an AssetRegistry) and keeps them loaded. An id that is unknown, whose
	// file is the wrong kind (a mesh asked for as a texture) or fails to load gives null, logged once.
	// Failures are remembered per kind: a file that isn't a texture still loads as the mesh it is.
	//
	// Not thread-safe: use it from the thread that records GPU work.
	class AssetLibrary
	{
	public:
		AssetLibrary(GfxBackendAPI& backend, AssetRegistry& registry);

		Ref<Mesh> GetMesh(AssetId id);
		RefCountPtr<ITexture2D> GetTexture(AssetId id);

		AssetRegistry& Registry() { return *m_Registry; }

	private:
		// The asset's path if it can be loaded as `kind`; otherwise records (and logs, once) why not
		std::optional<std::filesystem::path> PathFor(AssetId id, AssetKind kind, std::unordered_set<AssetId>& failed);

		GfxBackendAPI* m_Backend;
		AssetRegistry* m_Registry;
		std::unordered_map<AssetId, Ref<Mesh>> m_Meshes;
		std::unordered_map<AssetId, RefCountPtr<ITexture2D>> m_Textures;
		std::unordered_set<AssetId> m_FailedMeshes;
		std::unordered_set<AssetId> m_FailedTextures;
	};

}
