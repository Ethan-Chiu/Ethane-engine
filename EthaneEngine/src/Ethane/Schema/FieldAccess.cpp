#include "ethpch.h"
#include "Ethane/Schema/FieldAccess.h"

#include <cstring>

namespace Ethane {

	namespace {

		template<typename T>
		const T& As(const void* p) { return *static_cast<const T*>(p); }

		template<typename T>
		T& As(void* p) { return *static_cast<T*>(p); }

		// The FieldValue alternative for a non-struct FieldType (they share their order)
		template<size_t I = 0, typename Fn>
		decltype(auto) WithAlternative(FieldType type, Fn&& fn)
		{
			if constexpr (I + 1 < std::variant_size_v<FieldValue>)
			{
				if (static_cast<size_t>(type) != I)
					return WithAlternative<I + 1>(type, std::forward<Fn>(fn));
			}
			return fn(std::in_place_index<I>);
		}

		void VisitLeaves(const TypeInfo& type, FieldPath& path, uint32_t baseOffset, const LeafFieldVisitor& visit)
		{
			for (size_t i = 0; i < type.Fields.size(); i++)
			{
				const FieldInfo& field = type.Fields[i];
				path.Indices.push_back(static_cast<uint16_t>(i));
				if (field.Type == FieldType::Struct)
					VisitLeaves(*field.Struct, path, baseOffset + field.Offset, visit);
				else
					visit(path, field, baseOffset + field.Offset);
				path.Indices.pop_back();
			}
		}

	}

	std::optional<ResolvedField> Resolve(const TypeInfo& type, const FieldPath& path)
	{
		if (path.Indices.empty())
			return std::nullopt;

		const TypeInfo* current = &type;
		ResolvedField result;
		for (size_t depth = 0; depth < path.Indices.size(); depth++)
		{
			if (!current || path.Indices[depth] >= current->Fields.size())
				return std::nullopt;
			result.Field = &current->Fields[path.Indices[depth]];
			result.Offset += result.Field->Offset;
			current = result.Field->Struct;
		}
		return result;
	}

	std::optional<FieldPath> ParseFieldPath(const TypeInfo& type, std::string_view dotted)
	{
		FieldPath path;
		const TypeInfo* current = &type;
		while (true)
		{
			if (!current)
				return std::nullopt; // descending into a field that isn't a struct
			const size_t dot = dotted.find('.');
			const int32_t index = current->FieldIndex(dotted.substr(0, dot));
			if (index < 0)
				return std::nullopt;
			path.Indices.push_back(static_cast<uint16_t>(index));
			if (dot == std::string_view::npos)
				return path;
			current = current->Fields[index].Struct;
			dotted.remove_prefix(dot + 1);
		}
	}

	std::string FormatFieldPath(const TypeInfo& type, const FieldPath& path)
	{
		std::string result;
		const TypeInfo* current = &type;
		for (uint16_t index : path.Indices)
		{
			if (!current || index >= current->Fields.size())
				return result + (result.empty() ? "?" : ".?");
			if (!result.empty())
				result += '.';
			result += current->Fields[index].Name;
			current = current->Fields[index].Struct;
		}
		return result;
	}

	FieldValue ReadField(FieldType type, const void* field)
	{
		ETH_CORE_ASSERT(type != FieldType::Struct, "ReadField: struct fields have no single value");
		return WithAlternative(type, [&]<size_t I>(std::in_place_index_t<I>) -> FieldValue {
			using T = std::variant_alternative_t<I, FieldValue>;
			return FieldValue(std::in_place_index<I>, As<T>(field));
		});
	}

	bool WriteField(FieldType type, void* field, const FieldValue& value)
	{
		if (type == FieldType::Struct || value.index() != static_cast<size_t>(type))
			return false;
		return WithAlternative(type, [&]<size_t I>(std::in_place_index_t<I>) {
			using T = std::variant_alternative_t<I, FieldValue>;
			As<T>(field) = std::get<I>(value);
			return true;
		});
	}

	std::optional<FieldValue> GetField(const TypeInfo& type, const void* object, const FieldPath& path)
	{
		auto resolved = Resolve(type, path);
		if (!resolved || resolved->Field->Type == FieldType::Struct)
			return std::nullopt;
		return ReadField(resolved->Field->Type, static_cast<const std::byte*>(object) + resolved->Offset);
	}

	bool SetField(const TypeInfo& type, void* object, const FieldPath& path, const FieldValue& value)
	{
		auto resolved = Resolve(type, path);
		if (!resolved)
			return false;
		return WriteField(resolved->Field->Type, static_cast<std::byte*>(object) + resolved->Offset, value);
	}

	bool FieldsEqual(const TypeInfo& type, const void* a, const void* b)
	{
		bool equal = true;
		VisitLeafFields(type, [&](const FieldPath&, const FieldInfo& field, uint32_t offset) {
			if (!equal)
				return;
			const void* fa = static_cast<const std::byte*>(a) + offset;
			const void* fb = static_cast<const std::byte*>(b) + offset;
			equal = ReadField(field.Type, fa) == ReadField(field.Type, fb);
		});
		return equal;
	}

	void VisitLeafFields(const TypeInfo& type, const LeafFieldVisitor& visit)
	{
		FieldPath path;
		VisitLeaves(type, path, 0, visit);
	}

}
