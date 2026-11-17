#include "ContentBrowser.h"

#include "Ethane/Asset/AssetRegistry.h"

#include <algorithm>
#include <cctype>

namespace Ethane {

	namespace fs = std::filesystem;

	namespace {

		std::string Lower(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		// True if `path` is `root` or inside it (both normalized, relative)
		bool IsUnder(const fs::path& path, const fs::path& root)
		{
			auto p = path.begin(), r = root.begin();
			for (; r != root.end(); ++r, ++p)
				if (p == path.end() || *p != *r)
					return false;
			return true;
		}

	}

	ContentBrowser::ContentBrowser(AssetRegistry& registry, std::vector<fs::path> roots)
		: m_Registry(&registry)
	{
		for (fs::path& root : roots)
			m_Roots.push_back(root.lexically_normal());
		if (m_Roots.empty())
			m_Roots.emplace_back(".");
		m_Current = m_Roots.front();
	}

	ContentBrowser::Kind ContentBrowser::KindOf(const fs::path& path)
	{
		if (Lower(path.extension().string()) == ".stele")
			return Kind::Stele;
		switch (AssetRegistry::KindOf(path))
		{
			case AssetKind::Mesh: return Kind::Mesh;
			case AssetKind::Texture: return Kind::Texture;
			default: return Kind::Other;
		}
	}

	bool ContentBrowser::Navigate(const fs::path& directory)
	{
		const fs::path normal = directory.lexically_normal();
		std::error_code ec;
		if (!fs::is_directory(normal, ec))
			return false;
		for (size_t i = 0; i < m_Roots.size(); i++)
		{
			if (IsUnder(normal, m_Roots[i]))
			{
				m_Root = i;
				m_Current = normal;
				m_Stale = true;
				return true;
			}
		}
		return false;
	}

	void ContentBrowser::GoUp()
	{
		if (CanGoUp())
			Navigate(m_Current.parent_path());
	}

	std::vector<fs::path> ContentBrowser::Breadcrumbs() const
	{
		std::vector<fs::path> crumbs;
		for (fs::path folder = m_Current;; folder = folder.parent_path())
		{
			crumbs.push_back(folder);
			if (folder == Root() || folder.empty() || folder == folder.parent_path())
				break;
		}
		std::reverse(crumbs.begin(), crumbs.end());
		return crumbs;
	}

	void ContentBrowser::SetFilter(std::string filter)
	{
		if (filter != m_Filter)
		{
			m_Filter = std::move(filter);
			m_Stale = true;
		}
	}

	bool ContentBrowser::IsListed(const fs::directory_entry& entry) const
	{
		const std::string name = entry.path().filename().string();
		return !name.empty() && name[0] != '.' && !HiddenNames.contains(name);
	}

	std::span<const ContentBrowser::Entry> ContentBrowser::Entries()
	{
		// Adding or removing a file changes its folder's write time
		std::error_code ec;
		const fs::file_time_type writeTime = fs::last_write_time(m_Current, ec);
		if (m_Stale || writeTime != m_ScannedWriteTime)
		{
			Scan();
			m_ScannedWriteTime = writeTime;
			m_Stale = false;
		}
		return m_Entries;
	}

	void ContentBrowser::Scan()
	{
		m_Entries.clear();
		std::error_code ec;
		auto add = [&](const fs::directory_entry& item) {
			const bool folder = item.is_directory(ec);
			m_Entries.push_back(Entry{
				.Path = item.path().lexically_normal(),
				.Name = item.path().filename().string(),
				.Type = folder ? Kind::Folder : KindOf(item.path()) });
		};

		if (m_Filter.empty())
		{
			for (const fs::directory_entry& item : fs::directory_iterator(m_Current, ec))
				if (IsListed(item))
					add(item);
		}
		else
		{
			const std::string needle = Lower(m_Filter);
			fs::recursive_directory_iterator it(m_Current, ec), end;
			for (; it != end && m_Entries.size() < kMaxMatches; it.increment(ec))
			{
				if (!IsListed(*it))
				{
					if (it->is_directory(ec))
						it.disable_recursion_pending(); // not into hidden folders either
					continue;
				}
				if (Lower(it->path().filename().string()).find(needle) != std::string::npos)
					add(*it);
			}
		}

		std::sort(m_Entries.begin(), m_Entries.end(), [](const Entry& a, const Entry& b) {
			const bool aFolder = a.Type == Kind::Folder, bFolder = b.Type == Kind::Folder;
			if (aFolder != bFolder)
				return aFolder;
			return Lower(a.Name) < Lower(b.Name);
		});
	}

	AssetId ContentBrowser::Import(const Entry& entry)
	{
		return entry.Type == Kind::Folder ? AssetId{} : m_Registry->Import(entry.Path);
	}

	AssetId ContentBrowser::Find(const Entry& entry) const
	{
		return entry.Type == Kind::Folder ? AssetId{} : m_Registry->Find(entry.Path);
	}

}
