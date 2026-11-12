#include "ethpch.h"
#include "Ethane/Asset/AssetRegistry.h"

#include <yaml-cpp/yaml.h>

#include <charconv>
#include <fstream>
#include <random>

namespace Ethane {

	namespace {

		AssetId NewId()
		{
			thread_local std::mt19937_64 engine(std::random_device{}());
			uint64_t value = 0;
			while (value == 0)
				value = engine();
			return { value };
		}

	}

	std::string AssetRegistry::Normalize(const std::filesystem::path& path)
	{
		return path.lexically_normal().generic_string();
	}

	AssetId AssetRegistry::Import(const std::filesystem::path& path)
	{
		const std::string key = Normalize(path);
		std::lock_guard lock(m_Mutex);
		if (auto it = m_Ids.find(key); it != m_Ids.end())
			return it->second;

		AssetId id = NewId();
		while (m_Paths.contains(id))
			id = NewId();
		m_Paths[id] = key;
		m_Ids[key] = id;
		m_Dirty = true;
		return id;
	}

	AssetId AssetRegistry::Find(const std::filesystem::path& path) const
	{
		std::lock_guard lock(m_Mutex);
		auto it = m_Ids.find(Normalize(path));
		return it != m_Ids.end() ? it->second : AssetId{};
	}

	std::optional<std::filesystem::path> AssetRegistry::GetPath(AssetId id) const
	{
		std::lock_guard lock(m_Mutex);
		auto it = m_Paths.find(id);
		if (it == m_Paths.end())
			return std::nullopt;
		return std::filesystem::path(it->second);
	}

	bool AssetRegistry::Relocate(AssetId id, const std::filesystem::path& newPath)
	{
		const std::string key = Normalize(newPath);
		std::lock_guard lock(m_Mutex);
		auto it = m_Paths.find(id);
		if (it == m_Paths.end())
			return false;
		if (auto other = m_Ids.find(key); other != m_Ids.end() && other->second != id)
			return false; // the path already belongs to another asset
		m_Ids.erase(it->second);
		it->second = key;
		m_Ids[key] = id;
		m_Dirty = true;
		return true;
	}

	std::vector<std::pair<AssetId, std::filesystem::path>> AssetRegistry::Entries() const
	{
		std::vector<std::pair<std::string, AssetId>> sorted;
		{
			std::lock_guard lock(m_Mutex);
			sorted.assign(m_Ids.begin(), m_Ids.end());
		}
		std::sort(sorted.begin(), sorted.end());
		std::vector<std::pair<AssetId, std::filesystem::path>> entries;
		entries.reserve(sorted.size());
		for (auto& [path, id] : sorted)
			entries.emplace_back(id, std::filesystem::path(path));
		return entries;
	}

	size_t AssetRegistry::Size() const
	{
		std::lock_guard lock(m_Mutex);
		return m_Paths.size();
	}

	bool AssetRegistry::IsDirty() const
	{
		std::lock_guard lock(m_Mutex);
		return m_Dirty;
	}

	std::string AssetRegistry::ToYaml() const
	{
		std::vector<std::pair<std::string, AssetId>> entries;
		{
			std::lock_guard lock(m_Mutex);
			entries.assign(m_Ids.begin(), m_Ids.end());
		}
		std::sort(entries.begin(), entries.end()); // by path: stable, readable diffs

		YAML::Emitter out;
		out << YAML::BeginMap << YAML::Key << "Assets" << YAML::Value << YAML::BeginSeq;
		for (const auto& [path, id] : entries)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Id" << YAML::Value << fmt::format("{:016x}", id.Value);
			out << YAML::Key << "Path" << YAML::Value << path;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq << YAML::EndMap;
		return std::string(out.c_str()) + "\n";
	}

	bool AssetRegistry::FromYaml(std::string_view yaml, std::string* error)
	{
		auto fail = [&](std::string message) {
			if (error)
				*error = std::move(message);
			return false;
		};

		std::unordered_map<AssetId, std::string> paths;
		std::unordered_map<std::string, AssetId> ids;
		try
		{
			const YAML::Node root = YAML::Load(std::string(yaml));
			const YAML::Node assets = root["Assets"];
			if (!assets || !assets.IsSequence())
				return fail("no 'Assets' list");
			for (const YAML::Node& entry : assets)
			{
				const std::string idText = entry["Id"].as<std::string>("");
				const std::string path = Normalize(entry["Path"].as<std::string>(""));
				uint64_t value = 0;
				auto [end, ec] = std::from_chars(idText.data(), idText.data() + idText.size(), value, 16);
				if (ec != std::errc() || end != idText.data() + idText.size() || value == 0 || path.empty() || path == ".")
					return fail("invalid entry (Id '" + idText + "', Path '" + path + "')");
				if (paths.contains({ value }) || ids.contains(path))
					return fail("duplicate entry for " + path);
				paths[{ value }] = path;
				ids[path] = { value };
			}
		}
		catch (const YAML::Exception& e)
		{
			return fail(std::string("not valid YAML: ") + e.what());
		}

		std::lock_guard lock(m_Mutex);
		m_Paths = std::move(paths);
		m_Ids = std::move(ids);
		m_Dirty = false;
		return true;
	}

	bool AssetRegistry::Load(const std::filesystem::path& file, std::string* error)
	{
		std::ifstream in(file, std::ios::binary);
		if (!in)
		{
			if (error)
				*error = "can't open " + file.string();
			return false;
		}
		std::stringstream text;
		text << in.rdbuf();
		return FromYaml(text.str(), error);
	}

	bool AssetRegistry::Save(const std::filesystem::path& file)
	{
		std::filesystem::path temporary = file;
		temporary += ".tmp";
		{
			std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
			if (!out || !(out << ToYaml()) || !out.flush())
				return false;
		}
		std::error_code ec;
		std::filesystem::rename(temporary, file, ec);
		if (ec)
			return false;
		std::lock_guard lock(m_Mutex);
		m_Dirty = false;
		return true;
	}

}
