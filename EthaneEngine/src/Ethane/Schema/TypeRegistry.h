#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/Schema/TypeInfo.h"

#include <memory>
#include <new>
#include <typeindex>
#include <unordered_map>

namespace Ethane {

	class TypeRegistry;

	// Returned by TypeRegistry::Register to describe the type's fields:
	//     registry.Register<Transform>("Transform")
	//         .Field("Position", &Transform::Position)
	//         .Field("Rotation", &Transform::Rotation, FieldFlags::Angle);
	template<typename T>
	class TypeBuilder
	{
	public:
		TypeBuilder(TypeRegistry& registry, TypeInfo& info) : m_Registry(registry), m_Info(info) {}

		template<typename M>
		TypeBuilder& Field(std::string_view name, M T::* member, FieldFlags flags = FieldFlags::None, FieldMeta meta = {});

		const TypeInfo& Info() const { return m_Info; }

	private:
		TypeRegistry& m_Registry;
		TypeInfo& m_Info;
	};

	// Holds the descriptions of all registered types.
	// Registration is not thread-safe and is meant to happen at startup; after that any number of
	// threads may read concurrently.
	class TypeRegistry
	{
	public:
		TypeRegistry() = default;
		TypeRegistry(const TypeRegistry&) = delete;
		TypeRegistry& operator=(const TypeRegistry&) = delete;

		// The engine-wide registry (tests and tools may also use their own instances)
		static TypeRegistry& Global();

		template<typename T>
		TypeBuilder<T> Register(std::string_view name, uint32_t version = 1);

		const TypeInfo* Find(TypeId id) const;
		const TypeInfo* Find(std::string_view name) const { return Find(TypeId::FromName(name)); }

		template<typename T>
		const TypeInfo* Find() const
		{
			auto it = m_ByCppType.find(std::type_index(typeid(T)));
			return it != m_ByCppType.end() ? it->second : nullptr;
		}

		std::vector<const TypeInfo*> Types() const; // sorted by name

	private:
		TypeInfo& Add(std::type_index cppType, std::string_view name);

		std::unordered_map<TypeId, std::unique_ptr<TypeInfo>> m_Types;
		std::unordered_map<std::type_index, const TypeInfo*> m_ByCppType;

		template<typename T> friend class TypeBuilder;
	};

	// ------------------------------------------------------------------------------------------------

	template<typename T>
	TypeBuilder<T> TypeRegistry::Register(std::string_view name, uint32_t version)
	{
		static_assert(std::is_default_constructible_v<T> && std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>,
		              "registered types must be default-constructible and copyable");

		TypeInfo& info = Add(std::type_index(typeid(T)), name);
		info.Size = sizeof(T);
		info.Align = alignof(T);
		info.Version = version;
		info.Construct = [](void* dst) { new (dst) T(); };
		info.CopyConstruct = [](void* dst, const void* src) { new (dst) T(*static_cast<const T*>(src)); };
		info.CopyAssign = [](void* dst, const void* src) { *static_cast<T*>(dst) = *static_cast<const T*>(src); };
		info.Destroy = [](void* obj) { static_cast<T*>(obj)->~T(); };
		return TypeBuilder<T>(*this, info);
	}

	template<typename T>
	template<typename M>
	TypeBuilder<T>& TypeBuilder<T>::Field(std::string_view name, M T::* member, FieldFlags flags, FieldMeta meta)
	{
		ETH_CORE_ASSERT(!m_Info.FindField(name), (fmt::format("{}: field {} registered twice", m_Info.Name, name)));

		// Offset of the member, measured on uninitialized storage (no T is constructed)
		alignas(T) static std::byte storage[sizeof(T)];
		const T* object = reinterpret_cast<const T*>(storage);
		const auto offset = reinterpret_cast<const std::byte*>(&(object->*member)) - storage;

		FieldInfo& field = m_Info.Fields.emplace_back();
		field.Name = name;
		field.Type = FieldTypeOf<M>();
		field.Offset = static_cast<uint32_t>(offset);
		field.Size = sizeof(M);
		field.Flags = flags;
		field.Meta = meta;
		if constexpr (FieldTypeOf<M>() == FieldType::Struct)
		{
			field.Struct = m_Registry.template Find<M>();
			ETH_CORE_ASSERT(field.Struct, (fmt::format("{}.{}: register the struct type before using it as a field", m_Info.Name, name)));
		}
		return *this;
	}

}
