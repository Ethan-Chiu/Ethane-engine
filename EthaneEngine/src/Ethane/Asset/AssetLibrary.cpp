#include "ethpch.h"
#include "Ethane/Asset/AssetLibrary.h"

#include "Ethane/GfxBackend/GfxBackendAPI.h"

namespace Ethane {

	AssetLibrary::AssetLibrary(GfxBackendAPI& backend, AssetRegistry& registry, JobSystem* jobs)
		: m_Backend(&backend), m_Registry(&registry), m_Jobs(jobs)
	{
	}

	AssetLibrary::~AssetLibrary()
	{
		std::lock_guard lock(m_Mutex);
		// The jobs only write their own results, but let them finish rather than run on unobserved
		for (auto& [id, pending] : m_PendingMeshes)
			m_Jobs->Wait(pending.Job);
		for (auto& [id, pending] : m_PendingTextures)
			m_Jobs->Wait(pending.Job);
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

	// --- meshes ------------------------------------------------------------------------------------------

	void AssetLibrary::StartMesh(AssetId id, const std::filesystem::path& path)
	{
		auto result = std::make_shared<std::optional<MeshImport>>();
		JobHandle job = m_Jobs->Submit([result, file = path.string()] { *result = Mesh::Import(file); });
		m_PendingMeshes[id] = { std::move(job), std::move(result) };
	}

	Ref<Mesh> AssetLibrary::FinishMesh(AssetId id, std::optional<MeshImport>& meshImport)
	{
		Ref<Mesh> mesh = meshImport ? Mesh::Create(*m_Backend, *meshImport) : nullptr;
		if (!mesh)
		{
			m_FailedMeshes.insert(id); // Import logged why
			return nullptr;
		}
		m_Meshes[id] = mesh;
		return mesh;
	}

	Ref<Mesh> AssetLibrary::GetMesh(AssetId id)
	{
		std::lock_guard lock(m_Mutex);
		if (auto it = m_Meshes.find(id); it != m_Meshes.end())
			return it->second;
		if (!id.IsValid() || m_FailedMeshes.contains(id) || m_PendingMeshes.contains(id))
			return nullptr;

		auto path = PathFor(id, AssetKind::Mesh, m_FailedMeshes);
		if (!path)
			return nullptr;
		if (!m_Jobs)
		{
			std::optional<MeshImport> meshImport = Mesh::Import(path->string());
			return FinishMesh(id, meshImport);
		}
		StartMesh(id, *path);
		return nullptr;
	}

	Ref<Mesh> AssetLibrary::LoadMesh(AssetId id)
	{
		std::lock_guard lock(m_Mutex);
		if (auto pending = m_PendingMeshes.find(id); pending != m_PendingMeshes.end())
		{
			m_Jobs->Wait(pending->second.Job); // helps run queued jobs meanwhile
			auto result = std::move(pending->second.Result);
			m_PendingMeshes.erase(pending);
			return FinishMesh(id, *result);
		}
		if (auto it = m_Meshes.find(id); it != m_Meshes.end())
			return it->second;
		if (!id.IsValid() || m_FailedMeshes.contains(id))
			return nullptr;
		auto path = PathFor(id, AssetKind::Mesh, m_FailedMeshes);
		if (!path)
			return nullptr;
		std::optional<MeshImport> meshImport = Mesh::Import(path->string());
		return FinishMesh(id, meshImport);
	}

	// --- textures ----------------------------------------------------------------------------------------

	void AssetLibrary::StartTexture(AssetId id, const std::filesystem::path& path)
	{
		auto result = std::make_shared<std::optional<DecodedImage>>();
		JobHandle job = m_Jobs->Submit([result, file = path.string()] { *result = DecodeImage(file); });
		m_PendingTextures[id] = { std::move(job), std::move(result) };
	}

	RefCountPtr<ITexture2D> AssetLibrary::FinishTexture(AssetId id, std::optional<DecodedImage>& image)
	{
		RefCountPtr<ITexture2D> texture;
		if (image)
			texture = m_Backend->CreateTexture2DFromImage(*image);
		if (!texture)
		{
			m_FailedTextures.insert(id); // DecodeImage logged why
			return {};
		}
		m_Textures[id] = texture;
		return texture;
	}

	RefCountPtr<ITexture2D> AssetLibrary::GetTexture(AssetId id)
	{
		std::lock_guard lock(m_Mutex);
		if (auto it = m_Textures.find(id); it != m_Textures.end())
			return it->second;
		if (!id.IsValid() || m_FailedTextures.contains(id) || m_PendingTextures.contains(id))
			return {};

		auto path = PathFor(id, AssetKind::Texture, m_FailedTextures);
		if (!path)
			return {};
		if (!m_Jobs)
		{
			std::optional<DecodedImage> image = DecodeImage(path->string());
			return FinishTexture(id, image);
		}
		StartTexture(id, *path);
		return {};
	}

	RefCountPtr<ITexture2D> AssetLibrary::LoadTexture(AssetId id)
	{
		std::lock_guard lock(m_Mutex);
		if (auto pending = m_PendingTextures.find(id); pending != m_PendingTextures.end())
		{
			m_Jobs->Wait(pending->second.Job);
			auto result = std::move(pending->second.Result);
			m_PendingTextures.erase(pending);
			return FinishTexture(id, *result);
		}
		if (auto it = m_Textures.find(id); it != m_Textures.end())
			return it->second;
		if (!id.IsValid() || m_FailedTextures.contains(id))
			return {};
		auto path = PathFor(id, AssetKind::Texture, m_FailedTextures);
		if (!path)
			return {};
		std::optional<DecodedImage> image = DecodeImage(path->string());
		return FinishTexture(id, image);
	}

	// --- per frame ---------------------------------------------------------------------------------------

	void AssetLibrary::Update()
	{
		std::lock_guard lock(m_Mutex);
		for (auto it = m_PendingMeshes.begin(); it != m_PendingMeshes.end();)
		{
			if (!it->second.Job.IsDone())
			{
				++it;
				continue;
			}
			auto result = std::move(it->second.Result);
			const AssetId id = it->first;
			it = m_PendingMeshes.erase(it);
			FinishMesh(id, *result);
		}
		for (auto it = m_PendingTextures.begin(); it != m_PendingTextures.end();)
		{
			if (!it->second.Job.IsDone())
			{
				++it;
				continue;
			}
			auto result = std::move(it->second.Result);
			const AssetId id = it->first;
			it = m_PendingTextures.erase(it);
			FinishTexture(id, *result);
		}
	}

}
