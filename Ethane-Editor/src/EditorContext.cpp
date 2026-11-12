#include "EditorContext.h"

#include "Ethane/Asset/AssetLibrary.h"
#include "Ethane/Core/Log.h"
#include "Ethane/Stele/SteleSerializer.h"

namespace Ethane {

	EditorContext::EditorContext(AssetLibrary& assets, std::filesystem::path registryFile)
		: m_Assets(&assets), m_RegistryFile(std::move(registryFile))
	{
	}

	EditorContext::~EditorContext() = default;

	AssetRegistry& EditorContext::Registry()
	{
		return m_Assets->Registry();
	}

	OpenStele& EditorContext::Add(Scope<Stele> stele, std::string name, std::filesystem::path path)
	{
		auto open = CreateScope<OpenStele>();
		open->Name = std::move(name);
		open->Path = std::move(path);
		open->Source = std::move(stele);
		open->SavedVersion = open->Source->Read().Version();
		open->EditWorld = CreateScope<EnttWorld>(open->Source->Read());
		EnttWorld* world = open->EditWorld.get();
		open->Follow = open->Source->Subscribe([world](const ChangeSet& changes) { world->Apply(changes); });

		m_Steles.push_back(std::move(open));
		m_Active = m_Steles.size() - 1;
		return *m_Steles.back();
	}

	OpenStele& EditorContext::New(std::string name)
	{
		return Add(CreateScope<Stele>(), std::move(name), {});
	}

	OpenStele* EditorContext::Open(const std::filesystem::path& path, std::string& error)
	{
		std::error_code ec;
		const std::filesystem::path wanted = std::filesystem::weakly_canonical(path, ec);
		for (size_t i = 0; i < m_Steles.size(); i++)
		{
			const std::filesystem::path& existing = m_Steles[i]->Path;
			if (!existing.empty() && std::filesystem::weakly_canonical(existing, ec) == wanted)
			{
				m_Active = i;
				return m_Steles[i].get();
			}
		}

		auto stele = CreateScope<Stele>();
		SteleSerializer::LoadResult result = SteleSerializer::Load(path, *stele);
		if (!result)
		{
			error = result.Error;
			return nullptr;
		}
		for (const std::string& warning : result.Warnings)
			ETH_WARN("{}: {}", path.generic_string(), warning);
		return &Add(std::move(stele), path.stem().string(), path);
	}

	bool EditorContext::Save(OpenStele& stele, const std::filesystem::path& path, std::string& error)
	{
		const std::filesystem::path target = path.empty() ? stele.Path : path;
		if (target.empty())
		{
			error = "the stele has no file yet";
			return false;
		}
		std::error_code ec;
		if (target.has_parent_path())
			std::filesystem::create_directories(target.parent_path(), ec);

		const SteleSnapshot snapshot = stele.Source->Read();
		if (!SteleSerializer::Save(snapshot, target))
		{
			error = "couldn't write " + target.generic_string();
			return false;
		}
		stele.Path = target;
		stele.Name = target.stem().string();
		stele.SavedVersion = snapshot.Version();

		if (Registry().IsDirty() && !Registry().Save(m_RegistryFile))
		{
			error = "saved the stele, but couldn't write " + m_RegistryFile.generic_string();
			return false;
		}
		return true;
	}

	void EditorContext::Close(size_t index)
	{
		if (index >= m_Steles.size())
			return;
		m_Steles.erase(m_Steles.begin() + index);
		if (m_Active > index || m_Active >= m_Steles.size())
			m_Active = m_Active > 0 ? m_Active - 1 : 0;
	}

	void EditorContext::SetActive(size_t index)
	{
		if (index < m_Steles.size())
			m_Active = index;
	}

	CommitResult EditorContext::Commit(Transaction transaction)
	{
		OpenStele* stele = Active();
		if (!stele)
			return { .Ok = false, .Error = "no stele is open" };
		CommitResult result = stele->Source->Commit(std::move(transaction));
		if (!result)
			ETH_WARN("Edit refused: {}", result.Error);
		else if (result.Changes)
			stele->History.Record(*result.Changes);
		return result;
	}

	CommitResult EditorContext::Undo()
	{
		OpenStele* stele = Active();
		if (!stele || !stele->History.CanUndo())
			return { .Ok = false, .Error = "nothing to undo" };
		CommitResult result = stele->History.Undo(*stele->Source);
		if (!result)
			ETH_WARN("Undo failed: {}", result.Error);
		return result;
	}

	CommitResult EditorContext::Redo()
	{
		OpenStele* stele = Active();
		if (!stele || !stele->History.CanRedo())
			return { .Ok = false, .Error = "nothing to redo" };
		CommitResult result = stele->History.Redo(*stele->Source);
		if (!result)
			ETH_WARN("Redo failed: {}", result.Error);
		return result;
	}

}
