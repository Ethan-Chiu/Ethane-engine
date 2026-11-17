#pragma once

#include "Ethane/Schema/FieldAccess.h"
#include "Ethane/Stele/SteleSnapshot.h"
#include "Theme/EditorTheme.h"

#include <imgui.h>

#include <optional>

namespace Ethane {

	class EditorContext;

	// The selected element's name and components, generated from the schema: every registered type is
	// editable without code of its own. A widget per field type (and flags: Color, Angle, ReadOnly,
	// Hidden, RuntimeOnly; metadata ranges), each component a card, nested structs indented. Components can be added and
	// removed. Each change is a transaction; one continuous drag of a widget is one undo step.
	class InspectorPanel
	{
	public:
		static constexpr const char* kWindowName = "Inspector";

		void Draw(EditorContext& context, bool* open, const EditorTheme& theme);

	private:
		void DrawComponent(EditorContext& context, const SteleSnapshot& snapshot, ElementId id, const SteleElement::Component& component);
		void DrawFields(EditorContext& context, const SteleSnapshot& snapshot, ElementId id, const TypeInfo& component,
		                const TypeInfo& type, const void* data, FieldPath& path);
		std::optional<FieldValue> DrawField(EditorContext& context, const SteleSnapshot& snapshot, const FieldInfo& field, const void* data);
		void DrawAddComponent(EditorContext& context, const SteleElement& element);
		// `count` floats as X / Y / Z (/ W) fields, one group (so the gesture tracking sees it as one widget)
		bool AxisFields(int count, float* values, float speed, float min, float max, const char* format);
		// A ranged float as a field filled up to its value
		bool FillSlider(float& value, float min, float max);

		// Call right after each field widget, every frame: the merge key for a change made by it, one key
		// per press-and-drag (0 when the widget isn't held)
		uint64_t TrackGesture();

		const EditorTheme* m_Theme = nullptr;
		ImDrawListSplitter m_AxisSplitter; // X / Y / Z cell backgrounds under their fields
		uint64_t m_Gestures = 0;
		uint64_t m_CurrentGesture = 0;
		std::string m_NameEdit;
		ElementId m_NameEditFor;
	};

}
