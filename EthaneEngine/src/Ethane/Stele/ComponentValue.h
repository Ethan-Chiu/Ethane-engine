#pragma once

#include "Ethane/Schema/TypeInfo.h"

#include <memory>

namespace Ethane {

	class ComponentValue;
	using ComponentRef = std::shared_ptr<const ComponentValue>;

	// One component's data, type-erased: a heap block laid out as `Type()` describes.
	// Immutable once shared (snapshots, ops and undo history hold ComponentRefs); editing a component
	// means copying it, changing the copy and storing the new ref.
	class ComponentValue
	{
	public:
		static std::shared_ptr<ComponentValue> MakeDefault(const TypeInfo& type);
		static std::shared_ptr<ComponentValue> MakeCopy(const TypeInfo& type, const void* source);
		std::shared_ptr<ComponentValue> Clone() const { return MakeCopy(*m_Type, m_Data); }

		~ComponentValue();
		ComponentValue(const ComponentValue&) = delete;
		ComponentValue& operator=(const ComponentValue&) = delete;

		const TypeInfo& Type() const { return *m_Type; }
		const void* Data() const { return m_Data; }
		void* MutableData() { return m_Data; } // only before the value is shared

		template<typename T>
		const T& As() const { return *static_cast<const T*>(m_Data); }

	private:
		explicit ComponentValue(const TypeInfo& type);

		const TypeInfo* m_Type;
		void* m_Data;
	};

}
