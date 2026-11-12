#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/Stele/Stele.h"
#include "Ethane/Stele/UndoHistory.h"
#include "Ethane/World/Entt/EnttWorld.h"

#include <filesystem>
#include <string>
#include <vector>

namespace Ethane {

	class AssetLibrary;
	class AssetRegistry;

	// One stele open in the editor, with everything the editor keeps for it
	struct OpenStele
	{
		std::string Name;            // shown in the UI: the file name, or what New was given
		std::filesystem::path Path;  // empty until it's saved
		Scope<Stele> Source;
		Scope<EnttWorld> EditWorld;  // follows Source's change sets
		SteleSubscription Follow;
		UndoHistory History;         // this editor's own edits to it
		ElementId Selected;
		uint64_t SavedVersion = 0;   // the stele version last saved or loaded

		bool IsDirty() const { return Source->Read().Version() != SavedVersion; }
	};

	// What the editor's panels work on: the open steles and which one is active. Panels read snapshots
	// (Active()->Source->Read()) and change a stele only through Commit / Undo / Redo, so every edit can
	// be undone and reaches the stele's edit world as a change set.
	class EditorContext
	{
	public:
		// `registryFile`: where the asset registry is saved along with steles
		EditorContext(AssetLibrary& assets, std::filesystem::path registryFile);
		~EditorContext();

		// An empty stele, made active
		OpenStele& New(std::string name);
		// Makes it active if it's already open; null (and `error` set) if it can't be loaded
		OpenStele* Open(const std::filesystem::path& path, std::string& error);
		// To `path`, or the stele's own path if empty; saves the asset registry too if it changed
		bool Save(OpenStele& stele, const std::filesystem::path& path, std::string& error);
		void Close(size_t index);

		std::span<const Scope<OpenStele>> Steles() const { return m_Steles; }
		size_t ActiveIndex() const { return m_Active; }
		void SetActive(size_t index);
		OpenStele* Active() { return m_Active < m_Steles.size() ? m_Steles[m_Active].get() : nullptr; }

		// Commits to the active stele and records the change for undo; refusals are logged
		CommitResult Commit(Transaction transaction);
		CommitResult Undo();
		CommitResult Redo();

		AssetLibrary& Assets() { return *m_Assets; }
		AssetRegistry& Registry();

	private:
		OpenStele& Add(Scope<Stele> stele, std::string name, std::filesystem::path path);

		AssetLibrary* m_Assets;
		std::filesystem::path m_RegistryFile;
		std::vector<Scope<OpenStele>> m_Steles;
		size_t m_Active = 0;
	};

}
