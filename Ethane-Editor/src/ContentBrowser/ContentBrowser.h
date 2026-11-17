#pragma once

#include "Ethane/Schema/Ids.h"

#include <filesystem>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace Ethane {

	class AssetRegistry;

	// What the content browser shows and does, independent of how it's drawn (ContentBrowserPanel draws it
	// with ImGui; another UI could draw the same model).
	//
	// Browses folders under a set of roots (paths relative to the working directory, e.g. "assets"), never
	// above them. Files are classified by extension; importing one registers it in the AssetRegistry.
	class ContentBrowser
	{
	public:
		enum class Kind { Folder, Mesh, Texture, Stele, Other };

		struct Entry
		{
			std::filesystem::path Path; // relative to the working directory, like the roots
			std::string Name;
			Kind Type = Kind::Other;
		};

		ContentBrowser(AssetRegistry& registry, std::vector<std::filesystem::path> roots);

		std::span<const std::filesystem::path> Roots() const { return m_Roots; }
		const std::filesystem::path& Root() const { return m_Roots[m_Root]; }
		const std::filesystem::path& CurrentDirectory() const { return m_Current; }

		// Ignored (returns false) unless `directory` is a folder under one of the roots
		bool Navigate(const std::filesystem::path& directory);
		bool CanGoUp() const { return m_Current != Root(); }
		void GoUp();
		// The folders from the root down to the current one, each a path that Navigate accepts
		std::vector<std::filesystem::path> Breadcrumbs() const;

		// Case-insensitive part of a name. While set, the entries are the matches in the current folder and
		// everything below it (at most kMaxMatches).
		void SetFilter(std::string filter);
		const std::string& Filter() const { return m_Filter; }
		static constexpr size_t kMaxMatches = 500;

		// Folders first, then files, each alphabetically. Rescanned when the folder changed on disk.
		std::span<const Entry> Entries();
		void Refresh() { m_Stale = true; }

		// Registers the file as an asset (or finds it); null for folders
		AssetId Import(const Entry& entry);
		AssetId Find(const Entry& entry) const;

		static Kind KindOf(const std::filesystem::path& path);
		// Folder or file names never listed (".DS_Store", the shader cache, ...); dot-files are hidden too
		std::unordered_set<std::string> HiddenNames = { "cache", ".DS_Store" };

	private:
		void Scan();
		bool IsListed(const std::filesystem::directory_entry& entry) const;

		AssetRegistry* m_Registry;
		std::vector<std::filesystem::path> m_Roots;
		size_t m_Root = 0;
		std::filesystem::path m_Current;
		std::string m_Filter;
		std::vector<Entry> m_Entries;
		bool m_Stale = true;
		std::filesystem::file_time_type m_ScannedWriteTime{};
	};

}
