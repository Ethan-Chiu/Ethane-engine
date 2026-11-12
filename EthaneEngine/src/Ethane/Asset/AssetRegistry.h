#pragma once

#include "Ethane/Schema/Ids.h"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Ethane {

	// Which file each AssetId refers to. Steles store AssetIds, never paths, so moving or renaming a file
	// only changes its entry here. Saved as YAML:
	//
	//     Assets:
	//       - Id: 5b1e0c9a2f774d13
	//         Path: assets/models/CobblestoneFloor/CobblestoneFloor.obj
	//
	// Paths are kept as given (relative to the working directory, with '/' separators). Thread-safe.
	class AssetRegistry
	{
	public:
		// The asset's id, registering the path under a new random id if it isn't known yet
		AssetId Import(const std::filesystem::path& path);

		AssetId Find(const std::filesystem::path& path) const; // null if not registered
		std::optional<std::filesystem::path> GetPath(AssetId id) const;

		// Points an existing id at another file (after moving or renaming it)
		bool Relocate(AssetId id, const std::filesystem::path& newPath);

		// Every registered asset, sorted by path
		std::vector<std::pair<AssetId, std::filesystem::path>> Entries() const;

		size_t Size() const;
		bool IsDirty() const; // changed since the last Load / Save

		// Replaces the contents with the file's; false (and unchanged) if it can't be read
		bool Load(const std::filesystem::path& file, std::string* error = nullptr);
		bool Save(const std::filesystem::path& file);

		std::string ToYaml() const;
		bool FromYaml(std::string_view yaml, std::string* error = nullptr);

	private:
		static std::string Normalize(const std::filesystem::path& path);

		mutable std::mutex m_Mutex;
		std::unordered_map<AssetId, std::string> m_Paths;
		std::unordered_map<std::string, AssetId> m_Ids;
		bool m_Dirty = false;
	};

}
