#include "ethpch.h"
#include "Ethane/Stele/SteleSerializer.h"
#include "Ethane/Stele/OrderKey.h"

#include <yaml-cpp/yaml.h>

#include <charconv>
#include <fstream>
#include <limits>

namespace Ethane::SteleSerializer {

	namespace {

		std::string Hex(uint64_t value) { return fmt::format("{:016x}", value); }

		std::optional<uint64_t> ParseHex(const std::string& text)
		{
			uint64_t value = 0;
			auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
			if (error != std::errc() || end != text.data() + text.size())
				return std::nullopt;
			return value;
		}

		// --- writing ---------------------------------------------------------------------------------

		template<int N>
		void EmitVector(YAML::Emitter& out, const glm::vec<N, float>& v)
		{
			out << YAML::Flow << YAML::BeginSeq;
			for (int i = 0; i < N; i++)
				out << v[i];
			out << YAML::EndSeq;
		}

		void EmitValue(YAML::Emitter& out, FieldType type, const void* field)
		{
			const FieldValue value = ReadField(type, field);
			std::visit([&](const auto& v) {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4>)
					EmitVector(out, v);
				else if constexpr (std::is_same_v<T, glm::quat>)
					out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq; // x y z w
				else if constexpr (std::is_same_v<T, AssetId> || std::is_same_v<T, ElementId>)
					out << Hex(v.Value);
				else
					out << v;
			}, value);
		}

		void EmitFields(YAML::Emitter& out, const TypeInfo& type, const void* instance)
		{
			out << YAML::BeginMap;
			for (const FieldInfo& field : type.Fields)
			{
				if (HasFlag(field.Flags, FieldFlags::RuntimeOnly))
					continue;
				const void* data = static_cast<const std::byte*>(instance) + field.Offset;
				out << YAML::Key << field.Name << YAML::Value;
				if (field.Type == FieldType::Struct)
					EmitFields(out, *field.Struct, data);
				else
					EmitValue(out, field.Type, data);
			}
			out << YAML::EndMap;
		}

		// --- reading ---------------------------------------------------------------------------------

		template<int N>
		std::optional<glm::vec<N, float>> ParseVector(const YAML::Node& node)
		{
			if (!node.IsSequence() || node.size() != N)
				return std::nullopt;
			glm::vec<N, float> v;
			for (int i = 0; i < N; i++)
				v[i] = node[i].as<float>();
			return v;
		}

		std::optional<FieldValue> ParseValue(FieldType type, const YAML::Node& node)
		{
			try
			{
				switch (type)
				{
					case FieldType::Bool:   return node.as<bool>();
					case FieldType::Int32:  return node.as<int32_t>();
					case FieldType::UInt32: return node.as<uint32_t>();
					case FieldType::Int64:  return node.as<int64_t>();
					case FieldType::UInt64: return node.as<uint64_t>();
					case FieldType::Float:  return node.as<float>();
					case FieldType::Double: return node.as<double>();
					case FieldType::Vec2:   if (auto v = ParseVector<2>(node)) return *v; return std::nullopt;
					case FieldType::Vec3:   if (auto v = ParseVector<3>(node)) return *v; return std::nullopt;
					case FieldType::Vec4:   if (auto v = ParseVector<4>(node)) return *v; return std::nullopt;
					case FieldType::Quat:
						if (auto v = ParseVector<4>(node))
							return glm::quat(v->w, v->x, v->y, v->z); // stored x y z w
						return std::nullopt;
					case FieldType::String: return node.as<std::string>();
					case FieldType::Asset:  if (auto id = ParseHex(node.as<std::string>())) return AssetId{ *id }; return std::nullopt;
					case FieldType::Element: if (auto id = ParseHex(node.as<std::string>())) return ElementId{ *id }; return std::nullopt;
					case FieldType::Struct: return std::nullopt;
				}
			}
			catch (const YAML::Exception&)
			{
			}
			return std::nullopt;
		}

		void ReadFields(const YAML::Node& map, const TypeInfo& type, void* instance, const std::string& where, std::vector<std::string>& warnings)
		{
			if (!map || !map.IsMap())
			{
				warnings.push_back(where + ": expected a map of fields");
				return;
			}
			for (const auto& entry : map)
			{
				const std::string name = entry.first.as<std::string>();
				const FieldInfo* field = type.FindField(name);
				const std::string fieldWhere = where + "." + name;
				if (!field)
				{
					warnings.push_back(fieldWhere + ": no such field, skipped");
					continue;
				}
				void* data = static_cast<std::byte*>(instance) + field->Offset;
				if (field->Type == FieldType::Struct)
				{
					ReadFields(entry.second, *field->Struct, data, fieldWhere, warnings);
					continue;
				}
				auto value = ParseValue(field->Type, entry.second);
				if (!value)
					warnings.push_back(fmt::format("{}: not a valid {}, left at its default", fieldWhere, ToString(field->Type)));
				else
					WriteField(field->Type, data, *value);
			}
		}

		struct LoadedElement
		{
			ElementId Id;
			std::string Name;
			ElementId Parent;
			std::string Order;
			std::vector<std::pair<TypeId, ComponentRef>> Components;
		};

	}

	std::string ToYaml(const SteleSnapshot& snapshot)
	{
		// Versions of the types this stele uses, so a later loader can tell which layout it was saved with
		std::map<std::string, uint32_t> usedTypes;
		snapshot.ForEachDepthFirst([&](const SteleElement& element) {
			for (const SteleElement::Component& component : element.Components)
				usedTypes[component.Value->Type().Name] = component.Value->Type().Version;
		});

		YAML::Emitter out;
		out.SetFloatPrecision(std::numeric_limits<float>::max_digits10);
		out.SetDoublePrecision(std::numeric_limits<double>::max_digits10);

		out << YAML::BeginMap;
		out << YAML::Key << "Stele" << YAML::Value << kFormatVersion;
		out << YAML::Key << "Types" << YAML::Value << YAML::BeginMap;
		for (const auto& [name, version] : usedTypes)
			out << YAML::Key << name << YAML::Value << version;
		out << YAML::EndMap;

		out << YAML::Key << "Elements" << YAML::Value << YAML::BeginSeq;
		snapshot.ForEachDepthFirst([&](const SteleElement& element) {
			out << YAML::BeginMap;
			out << YAML::Key << "Id" << YAML::Value << Hex(element.Id.Value);
			out << YAML::Key << "Name" << YAML::Value << element.Name;
			if (element.Parent.IsValid())
				out << YAML::Key << "Parent" << YAML::Value << Hex(element.Parent.Value);
			out << YAML::Key << "Order" << YAML::Value << element.Order;
			if (!element.Components.empty())
			{
				// Sorted by name so files diff well (the stele sorts components by TypeId)
				std::vector<const SteleElement::Component*> components;
				for (const SteleElement::Component& component : element.Components)
					components.push_back(&component);
				std::sort(components.begin(), components.end(), [](const SteleElement::Component* a, const SteleElement::Component* b) {
					return a->Value->Type().Name < b->Value->Type().Name;
				});

				out << YAML::Key << "Components" << YAML::Value << YAML::BeginMap;
				for (const SteleElement::Component* component : components)
				{
					out << YAML::Key << component->Value->Type().Name << YAML::Value;
					EmitFields(out, component->Value->Type(), component->Value->Data());
				}
				out << YAML::EndMap;
			}
			out << YAML::EndMap;
		});
		out << YAML::EndSeq;
		out << YAML::EndMap;
		return std::string(out.c_str()) + "\n";
	}

	static LoadResult FromYamlUnguarded(std::string_view yaml, Stele& stele, std::string label)
	{
		LoadResult result;
		const TypeRegistry& registry = stele.Registry();

		YAML::Node root;
		try
		{
			root = YAML::Load(std::string(yaml));
		}
		catch (const YAML::Exception& e)
		{
			result.Error = std::string("not valid YAML: ") + e.what();
			return result;
		}
		if (!root.IsMap() || !root["Stele"])
		{
			result.Error = "not a stele file (no 'Stele' key)";
			return result;
		}
		if (root["Stele"].as<int>(0) > kFormatVersion)
		{
			result.Error = fmt::format("saved in stele format {}, this build reads up to {}", root["Stele"].as<int>(0), kFormatVersion);
			return result;
		}

		if (const YAML::Node types = root["Types"]; types && types.IsMap())
			for (const auto& entry : types)
			{
				const std::string name = entry.first.as<std::string>();
				const TypeInfo* type = registry.Find(name);
				const uint32_t saved = entry.second.as<uint32_t>(0);
				if (type && saved > type->Version)
					result.Warnings.push_back(fmt::format("{} was saved by a newer version (v{}, this build has v{}): fields it added are lost",
						name, saved, type->Version));
			}

		// Read every element first, then create them parents-first, whatever their order in the file
		std::vector<LoadedElement> elements;
		std::unordered_map<ElementId, size_t> index;
		const YAML::Node list = root["Elements"];
		if (list && !list.IsSequence())
		{
			result.Error = "'Elements' must be a list";
			return result;
		}
		for (const YAML::Node& node : list)
		{
			LoadedElement element;
			auto id = node["Id"] ? ParseHex(node["Id"].as<std::string>()) : std::nullopt;
			if (!id || *id == 0)
			{
				result.Error = fmt::format("element #{} has no valid Id", elements.size());
				return result;
			}
			element.Id = { *id };
			element.Name = node["Name"].as<std::string>("");
			if (node["Parent"])
				element.Parent = { ParseHex(node["Parent"].as<std::string>()).value_or(0) };
			element.Order = node["Order"].as<std::string>("");

			const std::string where = "element " + Hex(element.Id.Value);
			if (const YAML::Node components = node["Components"]; components && components.IsMap())
				for (const auto& entry : components)
				{
					const std::string typeName = entry.first.as<std::string>();
					const TypeInfo* type = registry.Find(typeName);
					if (!type)
					{
						result.Warnings.push_back(where + ": unknown component type " + typeName + ", skipped");
						continue;
					}
					auto value = ComponentValue::MakeDefault(*type);
					ReadFields(entry.second, *type, value->MutableData(), where + " " + typeName, result.Warnings);
					element.Components.emplace_back(type->Id, std::move(value));
				}

			if (index.contains(element.Id))
			{
				result.Error = where + " appears twice";
				return result;
			}
			index[element.Id] = elements.size();
			elements.push_back(std::move(element));
		}

		Transaction transaction(std::move(label), registry);
		std::vector<uint8_t> state(elements.size(), 0); // 0 new, 1 being created, 2 created
		std::string cycleError;
		std::function<void(size_t)> create = [&](size_t i) {
			if (state[i] == 2 || !cycleError.empty())
				return;
			if (state[i] == 1)
			{
				cycleError = "element " + Hex(elements[i].Id.Value) + " is its own ancestor";
				return;
			}
			state[i] = 1;
			LoadedElement& element = elements[i];
			if (element.Parent.IsValid())
			{
				auto parent = index.find(element.Parent);
				if (parent != index.end())
					create(parent->second);
				else if (!stele.Read().Contains(element.Parent))
				{
					result.Warnings.push_back("element " + Hex(element.Id.Value) + ": parent missing, loaded at the top level");
					element.Parent = {};
				}
			}
			if (!OrderKey::IsValid(element.Order))
			{
				result.Warnings.push_back("element " + Hex(element.Id.Value) + ": invalid order key, placed last");
				element.Order.clear();
			}
			transaction.CreateElement(element.Id, element.Name, element.Parent, element.Order);
			for (auto& [type, value] : element.Components)
				transaction.AddComponent(element.Id, type, std::move(value));
			state[i] = 2;
		};
		for (size_t i = 0; i < elements.size(); i++)
			create(i);
		if (!cycleError.empty())
		{
			result.Error = cycleError;
			return result;
		}

		CommitResult commit = stele.Commit(std::move(transaction));
		if (!commit)
		{
			result.Error = commit.Error;
			return result;
		}
		result.Ok = true;
		return result;
	}

	LoadResult FromYaml(std::string_view yaml, Stele& stele, std::string label)
	{
		try
		{
			return FromYamlUnguarded(yaml, stele, std::move(label));
		}
		catch (const YAML::Exception& e) // a value of the wrong shape where the file's structure is read
		{
			return { .Ok = false, .Error = std::string("malformed stele file: ") + e.what() };
		}
	}

	bool Save(const SteleSnapshot& snapshot, const std::filesystem::path& path)
	{
		// Write a temporary file and swap it in, so a failed save never leaves a half-written stele
		std::filesystem::path temporary = path;
		temporary += ".tmp";
		{
			std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
			if (!file)
				return false;
			file << ToYaml(snapshot);
			if (!file.flush())
				return false;
		}
		std::error_code error;
		std::filesystem::rename(temporary, path, error);
		return !error;
	}

	LoadResult Load(const std::filesystem::path& path, Stele& stele)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
			return { .Ok = false, .Error = "can't open " + path.string() };
		std::stringstream text;
		text << file.rdbuf();
		return FromYaml(text.str(), stele, "Load " + path.filename().string());
	}

}
