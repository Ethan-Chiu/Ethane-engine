#pragma once

#include "Ethane/Schema/FieldAccess.h"
#include "Ethane/Stele/SteleSnapshot.h"

#include <optional>

namespace Ethane {

	class EditorContext;

	// The selected element's name and components, generated from the schema: every registered type is
	// editable without code of its own. A widget per field type (and flags: Color, Angle, ReadOnly,
	// Hidden, RuntimeOnly; metadata ranges), nested structs as tree nodes. Components can be added and
	// removed. Each change is a transaction; one continuous drag of a widget is one undo step.
	class InspectorPanel
	{
	public:
		static constexpr const char* kWindowName = "Inspector";

		void Draw(EditorContext& context, bool* open);

	private:
		void DrawComponent(EditorContext& context, const SteleSnapshot& snapshot, ElementId id, const SteleElement::Component& component);
		void DrawFields(EditorContext& context, const SteleSnapshot& snapshot, ElementId id, const TypeInfo& component,
		                const TypeInfo& type, const void* data, FieldPath& path);
		std::optional<FieldValue> DrawField(EditorContext& context, const SteleSnapshot& snapshot, const FieldInfo& field, const void* data);
		void DrawAddComponent(EditorContext& context, const SteleElement& element);

		// The merge key for a change made by the last widget: one key per press-and-drag of a widget
		uint64_t GestureKey();

		uint64_t m_Gestures = 0;
		uint64_t m_CurrentGesture = 0;
		std::string m_NameEdit;
		ElementId m_NameEditFor;
	};

}
