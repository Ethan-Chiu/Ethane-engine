#include "ethpch.h"
#include "Ethane/Asset/AssetLibrary.h"

#include "Ethane/GfxBackend/GfxBackendAPI.h"
#include "Ethane/Renderer/Mesh.h"

namespace Ethane {

	AssetLibrary::AssetLibrary(GfxBackendAPI& backend, AssetRegistry& registry)
		: m_Backend(&backend), m_Registry(&registry)
	{
	}

	std::optional<std::filesystem::path> AssetLibrary::PathFor(AssetId id, AssetKind kind, std::unordered_set<AssetId>& failed)
	{
		const char* kindName = kind == AssetKind::Mesh ? "mesh" : "texture";
		auto path = m_Registry->GetPath(id);
		if (!path)
			ETH_CORE_WARN("Asset {:016x} isn't in the registry (wanted as a {})", id.Value, kindName);
		else if (AssetKind actual = AssetRegistry::KindOf(*path); actual != AssetKind::Any && actual != kind)
			ETH_CORE_WARN("{} isn't a {}", path->generic_string(), kindName);
		else
			return path;
		failed.insert(id);
		return std::nullopt;
	}

	Ref<Mesh> AssetLibrary::GetMesh(AssetId id)
	{
		if (auto it = m_Meshes.find(id); it != m_Meshes.end())
			return it->second;
		if (!id.IsValid() || m_FailedMeshes.contains(id))
			return nullptr;

		auto path = PathFor(id, AssetKind::Mesh, m_FailedMeshes);
		if (!path)
			return nullptr;
		Ref<Mesh> mesh = Mesh::Load(*m_Backend, path->string()); // logs why on failure
		if (!mesh)
		{
			m_FailedMeshes.insert(id);
			return nullptr;
		}
		m_Meshes[id] = mesh;
		return mesh;
	}

	RefCountPtr<ITexture2D> AssetLibrary::GetTexture(AssetId id)
	{
		if (auto it = m_Textures.find(id); it != m_Textures.end())
			return it->second;
		if (!id.IsValid() || m_FailedTextures.contains(id))
			return {};

		auto path = PathFor(id, AssetKind::Texture, m_FailedTextures);
		if (!path)
			return {};
		RefCountPtr<ITexture2D> texture = m_Backend->LoadTexture2D(path->string()); // logs why on failure
		if (!texture)
		{
			m_FailedTextures.insert(id);
			return {};
		}
		m_Textures[id] = texture;
		return texture;
	}

}
