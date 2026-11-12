#pragma once

#include "Ethane/Stele/SteleOps.h"
#include "Ethane/Schema/TypeRegistry.h"

#include <string>
#include <vector>

namespace Ethane {

	class SteleSnapshot;

	// A list of ops committed together: either all apply or none do.
	//
	//     Transaction tx("Add lamp");
	//     ElementId lamp = tx.CreateElement("Lamp");
	//     tx.AddComponent(lamp, Transform{ .Position = { 0, 2, 0 } });
	//     stele.Commit(std::move(tx));
	class Transaction
	{
	public:
		explicit Transaction(std::string label = {}, const TypeRegistry& registry = TypeRegistry::Global());

		// Shown in the undo history
		std::string Label;
		// Consecutive commits with the same non-zero key become one undo step (e.g. a slider drag)
		uint64_t MergeKey = 0;

		ElementId CreateElement(std::string name, ElementId parent = {}, std::string order = {});
		void CreateElement(ElementId id, std::string name, ElementId parent = {}, std::string order = {});
		void DestroyElement(ElementId id);
		void DestroyTree(const SteleSnapshot& snapshot, ElementId root); // the element and its descendants
		void Rename(ElementId id, std::string name);
		void Move(ElementId id, ElementId parent, std::string order = {});

		void AddComponent(ElementId id, TypeId type, ComponentRef value = nullptr);
		void RemoveComponent(ElementId id, TypeId type);
		void SetField(ElementId id, TypeId type, FieldPath path, FieldValue value);

		template<typename T>
		void AddComponent(ElementId id, const T& value = T())
		{
			const TypeInfo* type = TypeOf<T>();
			if (type)
				AddComponent(id, type->Id, ComponentValue::MakeCopy(*type, &value));
		}

		template<typename T>
		void RemoveComponent(ElementId id)
		{
			if (const TypeInfo* type = TypeOf<T>())
				RemoveComponent(id, type->Id);
		}

		// `field` is a dotted path such as "Bulb.Color"
		template<typename T>
		void SetField(ElementId id, std::string_view field, FieldValue value)
		{
			const TypeInfo* type = TypeOf<T>();
			if (!type)
				return;
			auto path = ParseFieldPath(*type, field);
			if (!path)
				return Fail("unknown field " + type->Name + "." + std::string(field));
			SetField(id, type->Id, std::move(*path), std::move(value));
		}

		void Append(SteleOperation op) { m_Ops.push_back(std::move(op)); }

		const std::vector<SteleOperation>& Ops() const { return m_Ops; }
		bool Empty() const { return m_Ops.empty(); }
		// Set when building the transaction went wrong (e.g. an unregistered type); Commit then refuses it
		const std::string& Error() const { return m_Error; }
		const TypeRegistry& Registry() const { return *m_Registry; }

	private:
		template<typename T>
		const TypeInfo* TypeOf()
		{
			const TypeInfo* type = m_Registry->Find<T>();
			if (!type)
				Fail(std::string("type is not registered: ") + typeid(T).name());
			return type;
		}

		void Fail(std::string error);

		const TypeRegistry* m_Registry;
		std::vector<SteleOperation> m_Ops;
		std::string m_Error;
	};

}
