#pragma once

#include "Ethane/Asset/AssetRegistry.h"
#include "Ethane/Asset/ImageLoader.h"
#include "Ethane/Core/Base.h"
#include "Ethane/Core/JobSystem.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Renderer/Mesh.h"

#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace Ethane {

	class GfxBackendAPI;

	// Loads assets by id (through an AssetRegistry) and keeps them loaded.
	//
	// With a JobSystem, Get* never blocks: the first call starts reading / decoding the file on a worker
	// and returns null; Update() (call it every frame) uploads finished ones to the GPU, after which Get*
	// returns them. Load* blocks until the asset is ready, for code that needs it right away. Without a
	// JobSystem everything loads on the calling thread.
	//
	// An id that is unknown, whose file is the wrong kind (a mesh asked for as a texture) or fails to load
	// gives null, logged once. Failures are remembered per kind: a file that isn't a texture still loads
	// as the mesh it is.
	//
	// Thread-safe: e.g. recording asks for meshes while the update thread calls Update(). Uploads happen on
	// the calling thread (they lock the device's upload pool and queue; lock order: this, then those).
	// Waits for its jobs when destroyed.
	class AssetLibrary
	{
	public:
		AssetLibrary(GfxBackendAPI& backend, AssetRegistry& registry, JobSystem* jobs = nullptr);
		~AssetLibrary();
		AssetLibrary(const AssetLibrary&) = delete;
		AssetLibrary& operator=(const AssetLibrary&) = delete;

		Ref<Mesh> GetMesh(AssetId id);
		RefCountPtr<ITexture2D> GetTexture(AssetId id);

		Ref<Mesh> LoadMesh(AssetId id);
		RefCountPtr<ITexture2D> LoadTexture(AssetId id);

		// Uploads the assets whose background loading finished
		void Update();
		size_t PendingCount() const
		{
			std::lock_guard lock(m_Mutex);
			return m_PendingMeshes.size() + m_PendingTextures.size();
		}

		AssetRegistry& Registry() { return *m_Registry; }

	private:
		template<typename T>
		struct Pending
		{
			JobHandle Job;
			std::shared_ptr<std::optional<T>> Result; // written by the job
		};

		// The asset's path if it can be loaded as `kind`; otherwise records (and logs, once) why not
		std::optional<std::filesystem::path> PathFor(AssetId id, AssetKind kind, std::unordered_set<AssetId>& failed);

		void StartMesh(AssetId id, const std::filesystem::path& path);
		void StartTexture(AssetId id, const std::filesystem::path& path);
		Ref<Mesh> FinishMesh(AssetId id, std::optional<MeshImport>& meshImport);
		RefCountPtr<ITexture2D> FinishTexture(AssetId id, std::optional<DecodedImage>& image);

		GfxBackendAPI* m_Backend;
		AssetRegistry* m_Registry; // thread-safe itself
		JobSystem* m_Jobs;
		// Guards everything below. The jobs never take it (they only fill their own results), so waiting
		// for one while holding it can't deadlock.
		mutable std::mutex m_Mutex;
		std::unordered_map<AssetId, Ref<Mesh>> m_Meshes;
		std::unordered_map<AssetId, RefCountPtr<ITexture2D>> m_Textures;
		std::unordered_map<AssetId, Pending<MeshImport>> m_PendingMeshes;
		std::unordered_map<AssetId, Pending<DecodedImage>> m_PendingTextures;
		std::unordered_set<AssetId> m_FailedMeshes;
		std::unordered_set<AssetId> m_FailedTextures;
	};

}
