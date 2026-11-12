#pragma once

#include <cstdint>

namespace Ethane {

	struct ChangeSet;
	class FramePacket;
	struct PacketView;

	struct UpdateContext
	{
		float DeltaTime = 0.0f; // seconds
	};

	// Edit: mirrors the stele for editing (no gameplay). Play: also runs gameplay (scripts).
	enum class WorldMode { Edit, Play };

	// A running simulation of one stele. The interface is deliberately coarse (no per-component calls):
	// each implementation stores its data however it likes, and nothing outside it sees that storage.
	//
	// A world follows its stele through ChangeSets: give it every ChangeSet of the stele in order. If one
	// doesn't follow on from the world's version (one was missed), the world rebuilds itself from the
	// snapshot the ChangeSet carries.
	class World
	{
	public:
		virtual ~World() = default;

		virtual void Apply(const ChangeSet& changes) = 0;
		virtual void Update(const UpdateContext& context) = 0;
		// Adds what there is to draw (meshes, lights) to the packet; the view is the caller's
		virtual void Extract(FramePacket& packet) const = 0;
		// The view through the world's primary camera; false if it has none
		virtual bool ExtractView(float aspectRatio, PacketView& view) const = 0;

		// The stele version the world currently reflects
		virtual uint64_t Version() const = 0;
	};

}
