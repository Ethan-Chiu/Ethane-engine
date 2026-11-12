#include "InspectorPanel.h"

#include "EditorContext.h"
#include "Ethane/Asset/AssetRegistry.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

namespace Ethane {

	namespace {

		std::string AssetLabel(AssetRegistry& registry, AssetId id)
		{
			if (!id.IsValid())
				return "(none)";
			auto path = registry.GetPath(id);
			return path ? path->filename().string() : fmt::format("missing {:016x}", id.Value);
		}

		std::string ElementLabel(const SteleSnapshot& snapshot, ElementId id)
		{
			if (!id.IsValid())
				return "(none)";
			const SteleElement* element = snapshot.Find(id);
			return element ? element->Name : fmt::format("missing {:016x}", id.Value);
		}

		bool HasRange(const FieldMeta& meta) { return meta.Min < meta.Max; }
		float Speed(const FieldMeta& meta, float fallback) { return meta.Speed > 0.0f ? meta.Speed : fallback; }

		template<typename T>
		bool DragScalar(const char* label, ImGuiDataType type, T& value, const FieldMeta& meta)
		{
			if (HasRange(meta))
			{
				const T min = static_cast<T>(meta.Min), max = static_cast<T>(meta.Max);
				return ImGui::DragScalar(label, type, &value, Speed(meta, 1.0f), &min, &max, nullptr, ImGuiSliderFlags_AlwaysClamp);
			}
			return ImGui::DragScalar(label, type, &value, Speed(meta, 1.0f));
		}

	}

	uint64_t InspectorPanel::GestureKey()
	{
		// A widget becoming active starts a gesture; changes while it stays active share its key, so a
		// drag is one undo step. A change from a click (no longer active) is a step of its own.
		if (ImGui::IsItemActivated())
			m_CurrentGesture = ++m_Gestures;
		return ImGui::IsItemActive() ? m_CurrentGesture : 0;
	}

	void InspectorPanel::Draw(EditorContext& context, bool* open)
	{
		ImGui::Begin(kWindowName, open);
		OpenStele* stele = context.Active();
		const SteleSnapshot snapshot = stele ? stele->Source->Read() : SteleSnapshot();
		const SteleElement* element = stele ? snapshot.Find(stele->Selected) : nullptr;
		if (!element)
		{
			ImGui::TextDisabled("Select an element in the Hierarchy");
			ImGui::End();
			return;
		}
		const ElementId id = element->Id;

		// Name: edited as text, committed when the field is left (or Enter)
		if (m_NameEditFor != id || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		{
			m_NameEdit = element->Name;
			m_NameEditFor = id;
		}
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##name", &m_NameEdit);
		if (ImGui::IsItemDeactivatedAfterEdit() && m_NameEdit != element->Name)
		{
			Transaction tx("Rename " + element->Name);
			tx.Rename(id, m_NameEdit);
			context.Commit(std::move(tx));
		}
		ImGui::TextDisabled("Element %016llx", static_cast<unsigned long long>(id.Value));

		// Components by type name (the stele keeps them by TypeId)
		std::vector<const SteleElement::Component*> components;
		for (const SteleElement::Component& component : element->Components)
			components.push_back(&component);
		std::sort(components.begin(), components.end(), [](auto* a, auto* b) { return a->Value->Type().Name < b->Value->Type().Name; });
		for (const SteleElement::Component* component : components)
			DrawComponent(context, snapshot, id, *component);

		ImGui::Spacing();
		DrawAddComponent(context, *element);
		ImGui::End();
	}

	void InspectorPanel::DrawComponent(EditorContext& context, const SteleSnapshot& snapshot, ElementId id, const SteleElement::Component& component)
	{
		const TypeInfo& type = component.Value->Type();
		ImGui::PushID(type.Name.c_str());
		const bool open = ImGui::CollapsingHeader(type.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);

		// Remove: from the header's context menu
		bool remove = false;
		if (ImGui::BeginPopupContextItem("##component"))
		{
			remove = ImGui::MenuItem("Remove component");
			ImGui::EndPopup();
		}

		if (open)
		{
			ImGui::Indent(4.0f);
			FieldPath path;
			DrawFields(context, snapshot, id, type, type, component.Value->Data(), path);
			ImGui::Unindent(4.0f);
		}
		ImGui::PopID();

		if (remove)
		{
			Transaction tx("Remove " + type.Name);
			tx.RemoveComponent(id, type.Id);
			context.Commit(std::move(tx));
		}
	}

	void InspectorPanel::DrawFields(EditorContext& context, const SteleSnapshot& snapshot, ElementId id, const TypeInfo& component,
	                                const TypeInfo& type, const void* data, FieldPath& path)
	{
		for (size_t i = 0; i < type.Fields.size(); i++)
		{
			const FieldInfo& field = type.Fields[i];
			if (HasFlag(field.Flags, FieldFlags::Hidden) || HasFlag(field.Flags, FieldFlags::RuntimeOnly))
				continue;
			const void* fieldData = static_cast<const std::byte*>(data) + field.Offset;
			path.Indices.push_back(static_cast<uint16_t>(i));
			ImGui::PushID(static_cast<int>(i));

			if (field.Type == FieldType::Struct)
			{
				if (ImGui::TreeNodeEx(field.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					DrawFields(context, snapshot, id, component, *field.Struct, fieldData, path);
					ImGui::TreePop();
				}
			}
			else
			{
				ImGui::BeginDisabled(HasFlag(field.Flags, FieldFlags::ReadOnly));
				if (auto value = DrawField(context, snapshot, field, fieldData))
				{
					Transaction tx("Edit " + component.Name + "." + FormatFieldPath(component, path));
					tx.MergeKey = GestureKey();
					tx.SetField(id, component.Id, path, std::move(*value));
					context.Commit(std::move(tx));
				}
				ImGui::EndDisabled();
			}

			ImGui::PopID();
			path.Indices.pop_back();
		}
	}

	std::optional<FieldValue> InspectorPanel::DrawField(EditorContext& context, const SteleSnapshot& snapshot, const FieldInfo& field, const void* data)
	{
		const char* label = field.Name.c_str();
		const FieldMeta& meta = field.Meta;
		const bool color = HasFlag(field.Flags, FieldFlags::Color);
		const bool angle = HasFlag(field.Flags, FieldFlags::Angle);
		const FieldValue current = ReadField(field.Type, data);

		switch (field.Type)
		{
			case FieldType::Bool:
			{
				bool v = std::get<bool>(current);
				if (ImGui::Checkbox(label, &v)) return v;
				break;
			}
			case FieldType::Int32:  { int32_t v = std::get<int32_t>(current);   if (DragScalar(label, ImGuiDataType_S32, v, meta)) return v; break; }
			case FieldType::UInt32: { uint32_t v = std::get<uint32_t>(current); if (DragScalar(label, ImGuiDataType_U32, v, meta)) return v; break; }
			case FieldType::Int64:  { int64_t v = std::get<int64_t>(current);   if (DragScalar(label, ImGuiDataType_S64, v, meta)) return v; break; }
			case FieldType::UInt64: { uint64_t v = std::get<uint64_t>(current); if (DragScalar(label, ImGuiDataType_U64, v, meta)) return v; break; }
			case FieldType::Double: { double v = std::get<double>(current);     if (DragScalar(label, ImGuiDataType_Double, v, meta)) return v; break; }
			case FieldType::Float:
			{
				float v = std::get<float>(current);
				if (angle)
				{
					float degrees = glm::degrees(v);
					if (ImGui::DragFloat(label, &degrees, 0.5f, 0.0f, 0.0f, "%.1f°")) return glm::radians(degrees);
				}
				else if (HasRange(meta) ? ImGui::SliderFloat(label, &v, meta.Min, meta.Max) : ImGui::DragFloat(label, &v, Speed(meta, 0.01f)))
					return v;
				break;
			}
			case FieldType::Vec2:
			{
				glm::vec2 v = std::get<glm::vec2>(current);
				if (ImGui::DragFloat2(label, glm::value_ptr(v), Speed(meta, 0.01f), meta.Min, meta.Max)) return v;
				break;
			}
			case FieldType::Vec3:
			{
				glm::vec3 v = std::get<glm::vec3>(current);
				if (color)
				{
					if (ImGui::ColorEdit3(label, glm::value_ptr(v), ImGuiColorEditFlags_Float)) return v;
				}
				else if (angle)
				{
					glm::vec3 degrees = glm::degrees(v);
					if (ImGui::DragFloat3(label, glm::value_ptr(degrees), 0.5f, 0.0f, 0.0f, "%.1f°")) return glm::radians(degrees);
				}
				else if (ImGui::DragFloat3(label, glm::value_ptr(v), Speed(meta, 0.01f), meta.Min, meta.Max))
					return v;
				break;
			}
			case FieldType::Vec4:
			{
				glm::vec4 v = std::get<glm::vec4>(current);
				if (color ? ImGui::ColorEdit4(label, glm::value_ptr(v), ImGuiColorEditFlags_Float)
				          : ImGui::DragFloat4(label, glm::value_ptr(v), Speed(meta, 0.01f), meta.Min, meta.Max))
					return v;
				break;
			}
			case FieldType::Quat:
			{
				// Edited as Euler angles in degrees
				const glm::quat q = std::get<glm::quat>(current);
				glm::vec3 degrees = glm::degrees(glm::eulerAngles(q));
				if (ImGui::DragFloat3(label, glm::value_ptr(degrees), 0.5f, 0.0f, 0.0f, "%.1f°")) return glm::quat(glm::radians(degrees));
				break;
			}
			case FieldType::String:
			{
				std::string v = std::get<std::string>(current);
				if (ImGui::InputText(label, &v)) return v;
				break;
			}
			case FieldType::Asset:
			{
				AssetRegistry& registry = context.Registry();
				const AssetId v = std::get<AssetId>(current);
				std::optional<FieldValue> chosen;
				if (ImGui::BeginCombo(label, AssetLabel(registry, v).c_str()))
				{
					if (ImGui::Selectable("(none)", !v.IsValid()))
						chosen = AssetId{};
					for (const auto& [asset, path] : registry.Entries())
						if (ImGui::Selectable(path.generic_string().c_str(), asset == v))
							chosen = asset;
					ImGui::EndCombo();
				}
				if (ImGui::IsItemHovered() && v.IsValid())
					if (auto path = registry.GetPath(v))
						ImGui::SetTooltip("%s", path->generic_string().c_str());
				return chosen;
			}
			case FieldType::Element:
			{
				const ElementId v = std::get<ElementId>(current);
				std::optional<FieldValue> chosen;
				if (ImGui::BeginCombo(label, ElementLabel(snapshot, v).c_str()))
				{
					if (ImGui::Selectable("(none)", !v.IsValid()))
						chosen = ElementId{};
					snapshot.ForEachDepthFirst([&](const SteleElement& element) {
						ImGui::PushID(static_cast<int>(element.Id.Value ^ (element.Id.Value >> 32)));
						if (ImGui::Selectable(element.Name.c_str(), element.Id == v))
							chosen = element.Id;
						ImGui::PopID();
					});
					ImGui::EndCombo();
				}
				return chosen;
			}
			case FieldType::Struct:
				break; // drawn by DrawFields
		}
		return std::nullopt;
	}

	void InspectorPanel::DrawAddComponent(EditorContext& context, const SteleElement& element)
	{
		const float width = ImGui::GetContentRegionAvail().x;
		if (ImGui::Button("Add component", ImVec2(width, 0.0f)))
			ImGui::OpenPopup("##add");
		if (!ImGui::BeginPopup("##add"))
			return;
		bool any = false;
		for (const TypeInfo* type : TypeRegistry::Global().Types())
		{
			if (element.FindComponent(type->Id))
				continue;
			any = true;
			if (ImGui::MenuItem(type->Name.c_str()))
			{
				Transaction tx("Add " + type->Name);
				tx.AddComponent(element.Id, type->Id);
				context.Commit(std::move(tx));
			}
		}
		if (!any)
			ImGui::TextDisabled("It has every component type");
		ImGui::EndPopup();
	}

}
