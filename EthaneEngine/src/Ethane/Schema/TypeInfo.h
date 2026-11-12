#pragma once

#include "Ethane/Schema/Ids.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace Ethane {

	// The value types a field can have. Order matches the alternatives of FieldValue (Struct excepted).
	enum class FieldType : uint8_t
	{
		Bool, Int32, UInt32, Int64, UInt64, Float, Double,
		Vec2, Vec3, Vec4, Quat,
		String, Asset, Element,
		Struct, // a nested registered type; its own fields hold the values
	};

	// A copy of one non-struct field's value, for type-erased reads and writes (ops, undo, inspector)
	using FieldValue = std::variant<
		bool, int32_t, uint32_t, int64_t, uint64_t, float, double,
		glm::vec2, glm::vec3, glm::vec4, glm::quat,
		std::string, AssetId, ElementId>;

	static_assert(std::variant_size_v<FieldValue> == static_cast<size_t>(FieldType::Struct));

	std::string_view ToString(FieldType type);

	// Which FieldType a C++ member type maps to; anything not listed must be a registered struct
	template<typename T>
	constexpr FieldType FieldTypeOf()
	{
		if constexpr (std::is_same_v<T, bool>) return FieldType::Bool;
		else if constexpr (std::is_same_v<T, int32_t>) return FieldType::Int32;
		else if constexpr (std::is_same_v<T, uint32_t>) return FieldType::UInt32;
		else if constexpr (std::is_same_v<T, int64_t>) return FieldType::Int64;
		else if constexpr (std::is_same_v<T, uint64_t>) return FieldType::UInt64;
		else if constexpr (std::is_same_v<T, float>) return FieldType::Float;
		else if constexpr (std::is_same_v<T, double>) return FieldType::Double;
		else if constexpr (std::is_same_v<T, glm::vec2>) return FieldType::Vec2;
		else if constexpr (std::is_same_v<T, glm::vec3>) return FieldType::Vec3;
		else if constexpr (std::is_same_v<T, glm::vec4>) return FieldType::Vec4;
		else if constexpr (std::is_same_v<T, glm::quat>) return FieldType::Quat;
		else if constexpr (std::is_same_v<T, std::string>) return FieldType::String;
		else if constexpr (std::is_same_v<T, AssetId>) return FieldType::Asset;
		else if constexpr (std::is_same_v<T, ElementId>) return FieldType::Element;
		else
		{
			static_assert(std::is_class_v<T>, "unsupported field type: use a value type or a registered struct");
			return FieldType::Struct;
		}
	}

	enum class FieldFlags : uint32_t
	{
		None        = 0,
		RuntimeOnly = 1 << 0, // not saved or synced; only meaningful inside a running world
		Hidden      = 1 << 1, // not shown in the inspector
		ReadOnly    = 1 << 2, // shown, not editable
		Color       = 1 << 3, // display hint: Vec3 / Vec4 is a color
		Angle       = 1 << 4, // display hint: radians, shown in degrees
	};

	constexpr FieldFlags operator|(FieldFlags a, FieldFlags b) { return FieldFlags(uint32_t(a) | uint32_t(b)); }
	constexpr FieldFlags operator&(FieldFlags a, FieldFlags b) { return FieldFlags(uint32_t(a) & uint32_t(b)); }
	constexpr bool HasFlag(FieldFlags flags, FieldFlags flag) { return (flags & flag) != FieldFlags::None; }

	// Editing hints; Min == Max means unbounded, Speed 0 means the inspector's default
	struct FieldMeta
	{
		float Min = 0.0f;
		float Max = 0.0f;
		float Speed = 0.0f;
		AssetKind Asset = AssetKind::Any; // Asset fields: which files fit (the inspector lists only those)
	};

	struct TypeInfo;

	struct FieldInfo
	{
		std::string Name;
		FieldType Type = FieldType::Bool;
		uint32_t Offset = 0;
		uint32_t Size = 0;
		FieldFlags Flags = FieldFlags::None;
		FieldMeta Meta;
		const TypeInfo* Struct = nullptr; // set for FieldType::Struct
	};

	// Description of a registered type. Owned by a TypeRegistry; pointers to it stay valid for the
	// registry's lifetime.
	struct TypeInfo
	{
		TypeId Id;
		std::string Name;
		uint32_t Size = 0;
		uint32_t Align = 0;
		uint32_t Version = 1; // bumped when the fields change in a way saved data must migrate from
		std::vector<FieldInfo> Fields;

		// Lifetime operations on raw, suitably aligned memory
		void (*Construct)(void* dst) = nullptr;
		void (*CopyConstruct)(void* dst, const void* src) = nullptr;
		void (*CopyAssign)(void* dst, const void* src) = nullptr;
		void (*Destroy)(void* obj) = nullptr;

		const FieldInfo* FindField(std::string_view name) const;
		int32_t FieldIndex(std::string_view name) const; // -1 if missing
	};

}
