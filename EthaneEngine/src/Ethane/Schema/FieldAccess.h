#pragma once

#include "Ethane/Schema/TypeInfo.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Ethane {

	// Path from a type to one of its fields, through nested structs: field indices, outermost first.
	// Indices are cheap to store in ops and undo history; names are used in files and messages.
	struct FieldPath
	{
		std::vector<uint16_t> Indices;

		bool operator==(const FieldPath&) const = default;
	};

	// A field located inside a type: where it is relative to the start of the outermost object
	struct ResolvedField
	{
		const FieldInfo* Field = nullptr;
		uint32_t Offset = 0;
	};

	std::optional<ResolvedField> Resolve(const TypeInfo& type, const FieldPath& path);

	// "Light.Color" <-> {index of Light, index of Color}
	std::optional<FieldPath> ParseFieldPath(const TypeInfo& type, std::string_view dotted);
	std::string FormatFieldPath(const TypeInfo& type, const FieldPath& path);

	// Copies a non-struct field's value out of / into memory laid out as `type` says
	FieldValue ReadField(FieldType type, const void* field);
	bool WriteField(FieldType type, void* field, const FieldValue& value); // false: value has another type

	std::optional<FieldValue> GetField(const TypeInfo& type, const void* object, const FieldPath& path);
	bool SetField(const TypeInfo& type, void* object, const FieldPath& path, const FieldValue& value);

	// True if both objects hold the same values in every registered field (nested structs included)
	bool FieldsEqual(const TypeInfo& type, const void* a, const void* b);

	// Calls `visit` for every non-struct field, depth-first in registration order
	using LeafFieldVisitor = std::function<void(const FieldPath& path, const FieldInfo& field, uint32_t offset)>;
	void VisitLeafFields(const TypeInfo& type, const LeafFieldVisitor& visit);

}
