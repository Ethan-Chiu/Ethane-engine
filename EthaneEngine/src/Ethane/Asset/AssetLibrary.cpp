#include "ethpch.h"
#include "Ethane/Asset/AssetLibrary.h"

#include "Ethane/GfxBackend/GfxBackendAPI.h"
#include "Ethane/Renderer/Mesh.h"

namespace Ethane {

	AssetLibrary::AssetLibrary(GfxBackendAPI& backend, AssetRegistry& registry)
		: m_Backend(&backend), m_Registry(&registry)
	{
	}

	bool AssetLibrary::Failed(AssetId id, const char* kind)
	{
		if (m_Failed.insert(id).second)
		{
			auto path = m_Registry->GetPath(id);
			ETH_CORE_WARN("{} asset {:016x} ({}) couldn't be loaded", kind, id.Value, path ? path->string() : "not in the registry");
		}
		return true;
	}

	Ref<Mesh> AssetLibrary::GetMesh(AssetId id)
	{
		if (!id.IsValid() || m_Failed.contains(id))
			return nullptr;
		if (auto it = m_Meshes.find(id); it != m_Meshes.end())
			return it->second;

		auto path = m_Registry->GetPath(id);
		Ref<Mesh> mesh = path ? Mesh::Load(*m_Backend, path->string()) : nullptr;
		if (!mesh)
		{
			Failed(id, "Mesh");
			return nullptr;
		}
		m_Meshes[id] = mesh;
		return mesh;
	}

	RefCountPtr<ITexture2D> AssetLibrary::GetTexture(AssetId id)
	{
		if (!id.IsValid() || m_Failed.contains(id))
			return {};
		if (auto it = m_Textures.find(id); it != m_Textures.end())
			return it->second;

		auto path = m_Registry->GetPath(id);
		RefCountPtr<ITexture2D> texture;
		if (path)
			texture = m_Backend->LoadTexture2D(path->string());
		if (!texture)
		{
			Failed(id, "Texture");
			return {};
		}
		m_Textures[id] = texture;
		return texture;
	}

}
