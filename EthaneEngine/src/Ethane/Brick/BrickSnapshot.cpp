#include "ethpch.h"
#include "Ethane/Brick/BrickSnapshot.h"

namespace Ethane {

	const BrickComponent* BrickObject::FindComponent(TypeId type) const
	{
		auto it = std::lower_bound(Components.begin(), Components.end(), type,
			[](const BrickComponent& c, TypeId t) { return c.Type < t; });
		return it != Components.end() && it->Type == type ? &*it : nullptr;
	}

	const BrickObject* BrickSnapshot::Find(ObjectId id) const
	{
		if (!m_State)
			return nullptr;
		auto it = m_State->Objects.find(id);
		return it != m_State->Objects.end() ? it->second.get() : nullptr;
	}

	std::span<const ObjectId> BrickSnapshot::Children(ObjectId parent) const
	{
		if (!m_State)
			return {};
		auto it = m_State->Children.find(parent);
		if (it == m_State->Children.end())
			return {};
		return { it->second->data(), it->second->size() };
	}

	const void* BrickSnapshot::GetComponent(ObjectId id, TypeId type) const
	{
		const BrickObject* object = Find(id);
		const BrickComponent* component = object ? object->FindComponent(type) : nullptr;
		return component ? component->Value->Data() : nullptr;
	}

}
