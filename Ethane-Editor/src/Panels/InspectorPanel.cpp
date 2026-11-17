#include "InspectorPanel.h"

#include "AssetDrop.h"
#include "EditorContext.h"
#include "Theme/ImGuiTheme.h"
#include "Theme/ImGuiWidgets.h"
#include "Ethane/Asset/AssetRegistry.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cctype>

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

		// Colors: a swatch and the hex value (click the swatch for the picker)
		constexpr ImGuiColorEditFlags kColorFlags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoOptions;

		// "BaseColorTexture" -> "Base color texture"
		std::string DisplayName(const std::string& name)
		{
			std::string out;
			for (size_t i = 0; i < name.size(); i++)
			{
				const char c = name[i];
				const bool upper = std::isupper(static_cast<unsigned char>(c)) != 0;
				if (i > 0 && upper && !std::isupper(static_cast<unsigned char>(name[i - 1])))
					out += ' ';
				out += i > 0 && upper ? static_cast<char>(std::tolower(static_cast<unsigned char>(c))) : c;
			}
			return out;
		}

		// `text` shortened with "..." to fit `width` in the current font
		std::string Elide(const std::string& text, float width)
		{
			if (ImGui::CalcTextSize(text.c_str()).x <= width)
				return text;
			std::string shortened = text;
			while (!shortened.empty() && ImGui::CalcTextSize((shortened + "...").c_str()).x > width)
				shortened.pop_back();
			return shortened + "...";
		}

		// A combo with a small chevron instead of ImGui's arrow button
		bool BeginCombo(const char* label, const char* preview)
		{
			const bool open = ImGui::BeginCombo(label, preview, ImGuiComboFlags_NoArrowButton);
			const ImVec2 min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
			DrawIcon(ImGui::GetWindowDrawList(), Icon::ChevronDown, ImVec2(max.x - 12.0f, (min.y + max.y) * 0.5f), 10.0f,
			         ImGui::GetColorU32(ImGuiCol_TextDisabled));
			return open;
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

	uint64_t InspectorPanel::TrackGesture()
	{
		// Called for every widget every frame, changed or not: a widget usually becomes active (mouse
		// press) a frame before its value first changes, and that's where the gesture must start.
		// Changes while the widget stays active share the gesture's key, so a drag is one undo step;
		// a change from a click (no longer active) is a step of its own. Multi-part widgets (X / Y / Z)
		// are groups, which report their parts' activity.
		if (ImGui::IsItemActivated())
			m_CurrentGesture = ++m_Gestures;
		return ImGui::IsItemActive() ? m_CurrentGesture : 0;
	}

	void InspectorPanel::Draw(EditorContext& context, bool* open, const EditorTheme& theme)
	{
		m_Theme = &theme;
		BeginPane(kWindowName, open, theme);
		OpenStele* stele = context.Active();
		const SteleSnapshot snapshot = stele ? stele->Source->Read() : SteleSnapshot();
		const SteleElement* element = stele ? snapshot.Find(stele->Selected) : nullptr;
		if (!element)
		{
			PaneTitle(kWindowName, theme);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
			ImGui::TextDisabled("Select an element");
			EndPane();
			return;
		}
		const ElementId id = element->Id;

		// Name: edited as text, committed when the field is left (or Enter)
		if (m_NameEditFor != id || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		{
			m_NameEdit = element->Name;
			m_NameEditFor = id;
		}
		ImGui::PushFont(EditorFonts().SemiBold, theme.FontSize + 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 7.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, theme.ControlRounding + 2.0f);
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##name", &m_NameEdit);
		ImGui::PopStyleVar(2);
		ImGui::PopFont();
		if (ImGui::IsItemDeactivatedAfterEdit() && m_NameEdit != element->Name)
		{
			Transaction tx("Rename " + element->Name);
			tx.Rename(id, m_NameEdit);
			context.Commit(std::move(tx));
		}
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
		MonoText(theme.TextFaint, "%016llx", static_cast<unsigned long long>(id.Value));
		ImGui::Dummy(ImVec2(0.0f, 4.0f));

		// Components by type name (the stele keeps them by TypeId)
		std::vector<const SteleElement::Component*> components;
		for (const SteleElement::Component& component : element->Components)
			components.push_back(&component);
		std::sort(components.begin(), components.end(), [](auto* a, auto* b) { return a->Value->Type().Name < b->Value->Type().Name; });
		for (const SteleElement::Component* component : components)
		{
			DrawComponent(context, snapshot, id, *component);
			ImGui::Dummy(ImVec2(0.0f, 4.0f));
		}

		DrawAddComponent(context, *element);
		EndPane();
	}

	void InspectorPanel::DrawComponent(EditorContext& context, const SteleSnapshot& snapshot, ElementId id, const SteleElement::Component& component)
	{
		const EditorTheme& theme = *m_Theme;
		const TypeInfo& type = component.Value->Type();
		ImGui::PushID(type.Name.c_str());
		BeginCard();

		// Header: click to fold, the "more" button (or a right click) for the component's menu
		bool& open = *ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("##open"), true);
		const float width = ImGui::GetContentRegionAvail().x, height = 32.0f;
		const ImVec2 min = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemAllowOverlap();
		if (ImGui::InvisibleButton("##header", ImVec2(width, height)))
			open = !open;
		const bool headerHovered = ImGui::IsItemHovered();
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			ImGui::OpenPopup("##component");
		ImDrawList* draw = ImGui::GetWindowDrawList();
		if (headerHovered)
			draw->AddRectFilled(min, ImVec2(min.x + width, min.y + height), ToImGuiU32(theme.Hover), theme.CardRounding,
			                    open ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll);
		DrawIcon(draw, open ? Icon::ChevronDown : Icon::ChevronRight, ImVec2(min.x + 16.0f, min.y + height * 0.5f), 10.0f, ToImGuiU32(theme.TextMuted));
		ImGui::PushFont(EditorFonts().SemiBold, 0.0f);
		draw->AddText(ImVec2(min.x + 28.0f, min.y + (height - ImGui::GetFontSize()) * 0.5f), ToImGuiU32(theme.Text), type.Name.c_str());
		ImGui::PopFont();
		ImGui::SetCursorScreenPos(ImVec2(min.x + width - 6.0f - 22.0f, min.y + (height - 22.0f) * 0.5f));
		if (IconButton("##more", Icon::More, 22.0f, EditorTheme::WithAlpha(theme.TextSecondary, 0.6f), theme, -1.0f, nullptr, 12.0f))
			ImGui::OpenPopup("##component");
		ImGui::SetCursorScreenPos(ImVec2(min.x, min.y + height));

		bool remove = false;
		if (ImGui::BeginPopup("##component"))
		{
			remove = ImGui::MenuItem("Remove component");
			ImGui::EndPopup();
		}

		if (open)
		{
			ImGui::Indent(10.0f);
			FieldPath path;
			DrawFields(context, snapshot, id, type, type, component.Value->Data(), path);
			ImGui::Unindent(10.0f);
			ImGui::Dummy(ImVec2(0.0f, 6.0f));
		}
		EndCard(theme);
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
		const EditorTheme& theme = *m_Theme;
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
				ImGui::TextColored(ToImGui(theme.TextSecondary), "%s", DisplayName(field.Name).c_str());
				ImGui::Indent(10.0f);
				DrawFields(context, snapshot, id, component, *field.Struct, fieldData, path);
				ImGui::Unindent(10.0f);
			}
			else
			{
				// Label on the left (shortened to fit, full name on hover), the widget filling the rest
				const float labelWidth = 84.0f, rowHeight = 24.0f;
				const float rowY = ImGui::GetCursorPosY();
				const std::string label = DisplayName(field.Name);
				ImGui::SetCursorPosY(rowY + (rowHeight - ImGui::GetFontSize()) * 0.5f);
				ImGui::PushFont(nullptr, theme.FontSize - 1.0f);
				ImGui::TextColored(ToImGui(theme.TextMuted), "%s", Elide(label, labelWidth - 6.0f).c_str());
				ImGui::PopFont();
				if (ImGui::IsItemHovered() && Elide(label, labelWidth - 6.0f) != label)
					ImGui::SetTooltip("%s", label.c_str());
				ImGui::SameLine(ImGui::GetCursorPosX() + labelWidth - ImGui::GetStyle().ItemSpacing.x + 0.0f);
				ImGui::SetCursorPosY(rowY);

				ImGui::BeginDisabled(HasFlag(field.Flags, FieldFlags::ReadOnly));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, (rowHeight - ImGui::GetFontSize()) * 0.5f));
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 10.0f);
				std::optional<FieldValue> value = DrawField(context, snapshot, field, fieldData);
				const uint64_t gesture = TrackGesture();
				ImGui::PopItemWidth();
				ImGui::PopStyleVar();
				if (value)
				{
					Transaction tx("Edit " + component.Name + "." + FormatFieldPath(component, path));
					tx.MergeKey = gesture;
					tx.SetField(id, component.Id, path, std::move(*value));
					context.Commit(std::move(tx));
				}
				ImGui::EndDisabled();
			}

			ImGui::PopID();
			path.Indices.pop_back();
		}
	}

	bool InspectorPanel::AxisFields(int count, float* values, float speed, float min, float max, const char* format)
	{
		const EditorTheme& theme = *m_Theme;
		const EditorTheme::Color axes[] = { theme.AxisX, theme.AxisY, theme.AxisZ, theme.TextSecondary };
		const char* names[] = { "X", "Y", "Z", "W" };
		const float gap = 3.0f;
		const float width = (ImGui::CalcItemWidth() - gap * (count - 1)) / count;
		const float letter = 14.0f;
		bool changed = false;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImGui::BeginGroup();
		ImGui::PushFont(EditorFonts().Mono, theme.FontSize - 1.5f);
		// Each cell: the axis letter in its own slot at the left, the drag field (centered text, no
		// background of its own) in the rest. The cell's background goes in a channel under both, colored by
		// the field's state.
		m_AxisSplitter.Split(draw, 2);
		const ImVec2 start = ImGui::GetCursorScreenPos();
		for (int i = 0; i < count; i++)
		{
			ImGui::PushID(i);
			m_AxisSplitter.SetCurrentChannel(draw, 1);
			const ImVec2 cell(start.x + i * (width + gap), start.y);
			ImGui::SetCursorScreenPos(ImVec2(cell.x + letter, cell.y));
			ImGui::SetNextItemWidth(width - letter);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
			changed |= ImGui::DragFloat("##axis", &values[i], speed, min, max, format);
			ImGui::PopStyleColor(3);
			const ImVec2 itemMax = ImGui::GetItemRectMax();
			const EditorTheme::Color& bg = ImGui::IsItemActive() ? theme.FieldActive : ImGui::IsItemHovered() ? theme.FieldHover : theme.Field;
			draw->AddText(ImVec2(cell.x + 6.0f, (cell.y + itemMax.y - ImGui::GetFontSize()) * 0.5f), ToImGuiU32(axes[i]), names[i]);
			m_AxisSplitter.SetCurrentChannel(draw, 0);
			draw->AddRectFilled(cell, itemMax, ToImGuiU32(bg), theme.ControlRounding);
			ImGui::PopID();
		}
		m_AxisSplitter.Merge(draw);
		ImGui::PopFont();
		ImGui::EndGroup();
		return changed;
	}

	bool InspectorPanel::FillSlider(float& value, float min, float max)
	{
		const EditorTheme& theme = *m_Theme;
		// ImGui's grab hidden; the fill (a wash with an edge) shows the value instead
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0, 0, 0, 0));
		ImGui::PushFont(EditorFonts().Mono, theme.FontSize - 1.5f);
		const bool changed = ImGui::SliderFloat("##slider", &value, min, max, "%.2f");
		ImGui::PopFont();
		ImGui::PopStyleColor(2);
		const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
		const float t = std::clamp((value - min) / std::max(max - min, 1e-6f), 0.0f, 1.0f);
		const float x = a.x + (b.x - a.x) * t;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		if (t > 0.0f)
			draw->AddRectFilled(a, ImVec2(x, b.y), ToImGuiU32(EditorTheme::WithAlpha(theme.Accent, 0.22f)), theme.ControlRounding, ImDrawFlags_RoundCornersLeft);
		draw->AddRectFilled(ImVec2(std::max(a.x, x - 2.0f), a.y), ImVec2(std::max(a.x + 2.0f, x), b.y), ToImGuiU32(theme.Accent));
		return changed;
	}

	std::optional<FieldValue> InspectorPanel::DrawField(EditorContext& context, const SteleSnapshot& snapshot, const FieldInfo& field, const void* data)
	{
		const char* label = "##v";
		const FieldMeta& meta = field.Meta;
		const bool color = HasFlag(field.Flags, FieldFlags::Color);
		const bool angle = HasFlag(field.Flags, FieldFlags::Angle);
		const FieldValue current = ReadField(field.Type, data);

		switch (field.Type)
		{
			case FieldType::Bool:
			{
				bool v = std::get<bool>(current);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
				if (Toggle(label, &v, *m_Theme)) return v;
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
				else if (HasRange(meta))
				{
					if (FillSlider(v, meta.Min, meta.Max)) return v;
				}
				else if (ImGui::DragFloat(label, &v, Speed(meta, 0.01f), 0.0f, 0.0f, "%.3f"))
					return v;
				break;
			}
			case FieldType::Vec2:
			{
				glm::vec2 v = std::get<glm::vec2>(current);
				if (AxisFields(2, glm::value_ptr(v), Speed(meta, 0.01f), meta.Min, meta.Max, "%.2f")) return v;
				break;
			}
			case FieldType::Vec3:
			{
				glm::vec3 v = std::get<glm::vec3>(current);
				if (color)
				{
					if (ImGui::ColorEdit3(label, glm::value_ptr(v), kColorFlags)) return v;
				}
				else if (angle)
				{
					glm::vec3 degrees = glm::degrees(v);
					if (AxisFields(3, glm::value_ptr(degrees), 0.5f, 0.0f, 0.0f, "%.1f°")) return glm::radians(degrees);
				}
				else if (AxisFields(3, glm::value_ptr(v), Speed(meta, 0.01f), meta.Min, meta.Max, "%.2f"))
					return v;
				break;
			}
			case FieldType::Vec4:
			{
				glm::vec4 v = std::get<glm::vec4>(current);
				if (color ? ImGui::ColorEdit4(label, glm::value_ptr(v), kColorFlags | ImGuiColorEditFlags_AlphaBar)
				          : AxisFields(4, glm::value_ptr(v), Speed(meta, 0.01f), meta.Min, meta.Max, "%.2f"))
					return v;
				break;
			}
			case FieldType::Quat:
			{
				// Edited as Euler angles in degrees
				const glm::quat q = std::get<glm::quat>(current);
				glm::vec3 degrees = glm::degrees(glm::eulerAngles(q));
				if (AxisFields(3, glm::value_ptr(degrees), 0.5f, 0.0f, 0.0f, "%.1f°")) return glm::quat(glm::radians(degrees));
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
				if (BeginCombo(label, AssetLabel(registry, v).c_str()))
				{
					if (ImGui::Selectable("(none)", !v.IsValid()))
						chosen = AssetId{};
					// Only files of the kind the field takes (a texture field lists images)
					for (const auto& [asset, path] : registry.Entries())
						if (meta.Asset == AssetKind::Any || AssetRegistry::KindOf(path) == meta.Asset)
							if (ImGui::Selectable(path.generic_string().c_str(), asset == v))
								chosen = asset;
					ImGui::EndCombo();
				}
				if (ImGui::IsItemHovered() && v.IsValid() && !ImGui::GetDragDropPayload())
					if (auto path = registry.GetPath(v))
						ImGui::SetTooltip("%s", path->generic_string().c_str());
				// Or drop one from the content browser
				if (auto dropped = AcceptAssetDrop(meta.Asset))
					chosen = dropped->Asset;
				return chosen;
			}
			case FieldType::Element:
			{
				const ElementId v = std::get<ElementId>(current);
				std::optional<FieldValue> chosen;
				if (BeginCombo(label, ElementLabel(snapshot, v).c_str()))
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
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, m_Theme->ControlRounding + 2.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ToImGui(m_Theme->TextSecondary));
		if (ImGui::Button("+  Add component", ImVec2(width, 30.0f)))
			ImGui::OpenPopup("##add");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
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
