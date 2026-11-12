#pragma once

#include "Ethane/Stele/ComponentValue.h"
#include "Ethane/Schema/TypeRegistry.h"

#include <memory>
#include <span>
#include <string>
#include <vector>

namespace Ethane {

	// One element of a stele: something with a name, a place in the hierarchy and components, which a
	// world turns into an entity. Immutable: a commit that changes an element stores a new SteleElement.
	struct SteleElement
	{
		struct Component
		{
			TypeId Type;
			ComponentRef Value;
		};

		ElementId Id;
		std::string Name;
		ElementId Parent;                 // null: top level
		std::string Order;                // position among its siblings (an OrderKey)
		std::vector<Component> Components; // sorted by TypeId

		const Component* FindComponent(TypeId type) const;
	};

	using SteleElementRef = std::shared_ptr<const SteleElement>;

	namespace Detail { struct SteleData; } // what a snapshot points to (SteleData.h, internal)

	// A read-only view of a stele at one version. Cheap to copy, safe to read from any thread, and never
	// changes: later commits create new snapshots and leave this one as it was.
	class SteleSnapshot
	{
	public:
		SteleSnapshot() = default;

		uint64_t Version() const;
		size_t ElementCount() const;
		const TypeRegistry& Registry() const;

		const SteleElement* Find(ElementId id) const;
		bool Contains(ElementId id) const { return Find(id) != nullptr; }

		// Children in order; the null id gives the top-level elements
		std::span<const ElementId> Children(ElementId parent = {}) const;

		// Raw component data, or null if the element or component is missing
		const void* GetComponent(ElementId id, TypeId type) const;

		template<typename T>
		const T* Get(ElementId id) const
		{
			const TypeInfo* type = Registry().Find<T>();
			return type ? static_cast<const T*>(GetComponent(id, type->Id)) : nullptr;
		}

		template<typename Fn> // Fn(const SteleElement&), parents before children, siblings in order
		void ForEachDepthFirst(Fn&& fn, ElementId root = {}) const
		{
			for (ElementId child : Children(root))
			{
				fn(*Find(child));
				ForEachDepthFirst(fn, child);
			}
		}

	private:
		explicit SteleSnapshot(std::shared_ptr<const Detail::SteleData> data) : m_Data(std::move(data)) {}

		std::shared_ptr<const Detail::SteleData> m_Data;
		friend class Stele;
	};

}
