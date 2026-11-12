#pragma once

#include "Ethane/World/World.h"

#include <memory>

namespace Ethane {

	class SteleSnapshot;

	// The default world: components in an EnTT registry, one entity per stele element.
	// EnTT is an implementation detail: it's included only by files in this folder.
	//
	// Stores the engine components (World/Components.h) and runs no gameplay systems yet, so it is what an
	// editor needs: it follows the stele and extracts what to draw, with the transform hierarchy applied.
	// Component types it doesn't know are ignored.
	class EnttWorld final : public World
	{
	public:
		explicit EnttWorld(const SteleSnapshot& snapshot);
		~EnttWorld() override;

		void Apply(const ChangeSet& changes) override;
		void Update(const UpdateContext& context) override;
		void Extract(FramePacket& packet) const override;
		uint64_t Version() const override;

		size_t EntityCount() const;
		uint32_t RebuildCount() const; // times it had to rebuild from a snapshot (missed ChangeSets)

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};

}
