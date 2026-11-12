#include "ethpch.h"
#include "Ethane/Schema/TypeRegistry.h"

#include <random>

namespace Ethane {

	ElementId ElementId::Generate()
	{
		thread_local std::mt19937_64 engine(std::random_device{}());
		uint64_t value = 0;
		while (value == 0)
			value = engine();
		return { value };
	}

	std::string_view ToString(FieldType type)
	{
		switch (type)
		{
			case FieldType::Bool:   return "Bool";
			case FieldType::Int32:  return "Int32";
			case FieldType::UInt32: return "UInt32";
			case FieldType::Int64:  return "Int64";
			case FieldType::UInt64: return "UInt64";
			case FieldType::Float:  return "Float";
			case FieldType::Double: return "Double";
			case FieldType::Vec2:   return "Vec2";
			case FieldType::Vec3:   return "Vec3";
			case FieldType::Vec4:   return "Vec4";
			case FieldType::Quat:   return "Quat";
			case FieldType::String: return "String";
			case FieldType::Asset:  return "Asset";
			case FieldType::Element: return "Element";
			case FieldType::Struct: return "Struct";
		}
		return "?";
	}

	const FieldInfo* TypeInfo::FindField(std::string_view name) const
	{
		const int32_t index = FieldIndex(name);
		return index >= 0 ? &Fields[index] : nullptr;
	}

	int32_t TypeInfo::FieldIndex(std::string_view name) const
	{
		for (size_t i = 0; i < Fields.size(); i++)
			if (Fields[i].Name == name)
				return static_cast<int32_t>(i);
		return -1;
	}

	TypeRegistry& TypeRegistry::Global()
	{
		static TypeRegistry registry;
		return registry;
	}

	TypeInfo& TypeRegistry::Add(std::type_index cppType, std::string_view name)
	{
		const TypeId id = TypeId::FromName(name);
		ETH_CORE_ASSERT(!m_ByCppType.contains(cppType), (fmt::format("type {} registered twice", name)));
		ETH_CORE_ASSERT(!m_Types.contains(id), (fmt::format("type name {} is taken (or its hash collides)", name)));

		auto info = std::make_unique<TypeInfo>();
		info->Id = id;
		info->Name = name;
		TypeInfo& ref = *info;
		m_Types[id] = std::move(info);
		m_ByCppType[cppType] = &ref;
		return ref;
	}

	const TypeInfo* TypeRegistry::Find(TypeId id) const
	{
		auto it = m_Types.find(id);
		return it != m_Types.end() ? it->second.get() : nullptr;
	}

	std::vector<const TypeInfo*> TypeRegistry::Types() const
	{
		std::vector<const TypeInfo*> types;
		types.reserve(m_Types.size());
		for (const auto& [id, info] : m_Types)
			types.push_back(info.get());
		std::sort(types.begin(), types.end(), [](const TypeInfo* a, const TypeInfo* b) { return a->Name < b->Name; });
		return types;
	}

}
