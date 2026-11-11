#pragma once

#include "Ethane/Brick/ComponentValue.h"
#include "Ethane/Schema/TypeRegistry.h"

#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace Ethane {

	struct BrickComponent
	{
		TypeId Type;
		ComponentRef Value;
	};

	// One object of a brick. Immutable: a commit that changes an object stores a new BrickObject.
	struct BrickObject
	{
		ObjectId Id;
		std::string Name;
		ObjectId Parent;
		std::string Order;
		std::vector<BrickComponent> Components; // sorted by TypeId

		const BrickComponent* FindComponent(TypeId type) const;
	};

	using BrickObjectRef = std::shared_ptr<const BrickObject>;

	namespace Detail {

		// Storage behind a snapshot; shared between consecutive snapshots wherever nothing changed
		struct BrickState
		{
			const TypeRegistry* Registry = nullptr;
			uint64_t Version = 0;
			std::unordered_map<ObjectId, BrickObjectRef> Objects;
			// Children of each parent (null id: top level), sorted by (Order, Id)
			std::unordered_map<ObjectId, std::shared_ptr<const std::vector<ObjectId>>> Children;
		};

	}

	// A read-only view of a brick at one version. Cheap to copy, safe to read from any thread, and never
	// changes: later commits create new snapshots and leave this one as it was.
	class BrickSnapshot
	{
	public:
		BrickSnapshot() = default;
		explicit BrickSnapshot(std::shared_ptr<const Detail::BrickState> state) : m_State(std::move(state)) {}

		uint64_t Version() const { return m_State ? m_State->Version : 0; }
		size_t ObjectCount() const { return m_State ? m_State->Objects.size() : 0; }
		const TypeRegistry& Registry() const { return *m_State->Registry; }

		const BrickObject* Find(ObjectId id) const;
		bool Contains(ObjectId id) const { return Find(id) != nullptr; }

		// Children in order; the null id gives the top-level objects
		std::span<const ObjectId> Children(ObjectId parent = {}) const;

		// Raw component data, or null if the object or component is missing
		const void* GetComponent(ObjectId id, TypeId type) const;

		template<typename T>
		const T* Get(ObjectId id) const
		{
			const TypeInfo* type = Registry().Find<T>();
			return type ? static_cast<const T*>(GetComponent(id, type->Id)) : nullptr;
		}

		template<typename Fn> // Fn(const BrickObject&), parents before children, siblings in order
		void ForEachDepthFirst(Fn&& fn, ObjectId root = {}) const
		{
			for (ObjectId child : Children(root))
			{
				fn(*Find(child));
				ForEachDepthFirst(fn, child);
			}
		}

		const Detail::BrickState* State() const { return m_State.get(); }

	private:
		std::shared_ptr<const Detail::BrickState> m_State;
	};

}
