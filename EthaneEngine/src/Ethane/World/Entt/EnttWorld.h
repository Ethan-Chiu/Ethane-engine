#pragma once

#include "Ethane/World/World.h"

#include <memory>

namespace Ethane {

	class ScriptRegistry;
	class SteleSnapshot;

	// The default world: components in an EnTT registry, one entity per stele element.
	// EnTT is an implementation detail: it's included only by files in this folder.
	//
	// Stores every registered component type (the engine's typed, others as copies of their
	// ComponentValue), so scripts can reach game components too. Combines the transform hierarchy when
	// extracting. In Play mode it also runs NativeScript components: a script is created on the first
	// Update after its component appears (OnCreate), updated every Update, and destroyed (OnDestroy)
	// when its element, component or class goes away, or with the world.
	class EnttWorld final : public World
	{
	public:
		// Play worlds create scripts from `scripts` (ScriptRegistry::Global() if null)
		explicit EnttWorld(const SteleSnapshot& snapshot, WorldMode mode = WorldMode::Edit, const ScriptRegistry* scripts = nullptr);
		~EnttWorld() override;

		void Apply(const ChangeSet& changes) override;
		void Update(const UpdateContext& context) override;
		void Extract(FramePacket& packet) const override;
		bool ExtractView(float aspectRatio, PacketView& view) const override;
		uint64_t Version() const override;

		WorldMode Mode() const;
		size_t EntityCount() const;
		size_t ScriptCount() const;    // live script instances
		uint32_t RebuildCount() const; // times it had to rebuild from a snapshot (missed ChangeSets)

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};

}
