#include "ethpch.h"
#include "Ethane/Stele/SteleSnapshot.h"
#include "Ethane/Stele/SteleData.h"

namespace Ethane {

	const SteleElement::Component* SteleElement::FindComponent(TypeId type) const
	{
		auto it = std::lower_bound(Components.begin(), Components.end(), type,
			[](const Component& c, TypeId t) { return c.Type < t; });
		return it != Components.end() && it->Type == type ? &*it : nullptr;
	}

	uint64_t SteleSnapshot::Version() const
	{
		return m_Data ? m_Data->Version : 0;
	}

	size_t SteleSnapshot::ElementCount() const
	{
		return m_Data ? m_Data->Elements.size() : 0;
	}

	const TypeRegistry& SteleSnapshot::Registry() const
	{
		return *m_Data->Registry;
	}

	const SteleElement* SteleSnapshot::Find(ElementId id) const
	{
		if (!m_Data)
			return nullptr;
		auto it = m_Data->Elements.find(id);
		return it != m_Data->Elements.end() ? it->second.get() : nullptr;
	}

	std::span<const ElementId> SteleSnapshot::Children(ElementId parent) const
	{
		if (!m_Data)
			return {};
		auto it = m_Data->Children.find(parent);
		if (it == m_Data->Children.end())
			return {};
		return { it->second->data(), it->second->size() };
	}

	const void* SteleSnapshot::GetComponent(ElementId id, TypeId type) const
	{
		const SteleElement* element = Find(id);
		const SteleElement::Component* component = element ? element->FindComponent(type) : nullptr;
		return component ? component->Value->Data() : nullptr;
	}

}
