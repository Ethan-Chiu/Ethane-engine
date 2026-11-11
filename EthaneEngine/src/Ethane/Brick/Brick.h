#pragma once

#include "Ethane/Brick/BrickSnapshot.h"
#include "Ethane/Brick/Transaction.h"

#include <functional>
#include <mutex>

namespace Ethane {

	// What one commit changed: published to subscribers (worlds, panels, sync) and recorded for undo
	struct ChangeSet
	{
		uint64_t FromVersion = 0;
		uint64_t ToVersion = 0;
		std::string Label;
		uint64_t MergeKey = 0;
		std::vector<BrickOperation> Ops;     // as applied: order keys and default components filled in
		std::vector<BrickOperation> Inverse; // ops that undo this change, in the order to apply them
		BrickSnapshot After;                 // the brick right after this commit
	};

	struct CommitResult
	{
		bool Ok = false;
		std::string Error;        // why the transaction was refused; the brick is unchanged then
		uint64_t Version = 0;     // the brick's version after the commit
		std::shared_ptr<const ChangeSet> Changes; // null if refused or empty

		explicit operator bool() const { return Ok; }
	};

	namespace Detail {

		struct BrickListeners
		{
			std::mutex Mutex;
			std::vector<std::pair<uint64_t, std::function<void(const ChangeSet&)>>> List;
			uint64_t NextId = 1;
		};

	}

	// Keeps a subscription alive; unsubscribes when destroyed (fine to outlive the brick)
	class BrickSubscription
	{
	public:
		BrickSubscription() = default;
		BrickSubscription(BrickSubscription&&) noexcept = default;
		BrickSubscription& operator=(BrickSubscription&& other) noexcept;
		~BrickSubscription() { Reset(); }

		void Reset();

	private:
		BrickSubscription(std::weak_ptr<Detail::BrickListeners> listeners, uint64_t id)
			: m_Listeners(std::move(listeners)), m_Id(id) {}

		std::weak_ptr<Detail::BrickListeners> m_Listeners;
		uint64_t m_Id = 0;
		friend class Brick;
	};

	// One unit of authored content (a level, a prefab, a preview setup): objects with names, a hierarchy
	// and components, as immutable snapshots changed only by committing transactions.
	//
	// Thread safety: Read() and Commit() may be called from any thread. Commits are serialized; each
	// commit's subscribers run on the committing thread, in commit order, and must not commit to the same
	// brick from inside the callback.
	class Brick
	{
	public:
		explicit Brick(const TypeRegistry& registry = TypeRegistry::Global());
		Brick(const Brick&) = delete;
		Brick& operator=(const Brick&) = delete;

		BrickSnapshot Read() const;

		CommitResult Commit(Transaction transaction);

		using Listener = std::function<void(const ChangeSet&)>;
		[[nodiscard]] BrickSubscription Subscribe(Listener listener);

		const TypeRegistry& Registry() const { return *m_Registry; }

	private:
		const TypeRegistry* m_Registry;

		mutable std::mutex m_SnapshotMutex; // guards m_Snapshot (held only to copy or swap it)
		BrickSnapshot m_Snapshot;

		std::mutex m_CommitMutex;           // one commit at a time, so listeners see commits in order
		std::shared_ptr<Detail::BrickListeners> m_Listeners;
	};

}
