#pragma once

#include "Ethane/Brick/BrickOps.h"
#include "Ethane/Schema/TypeRegistry.h"

#include <string>
#include <vector>

namespace Ethane {

	class BrickSnapshot;

	// A list of ops committed together: either all apply or none do.
	//
	//     Transaction tx("Add lamp");
	//     ObjectId lamp = tx.CreateObject("Lamp");
	//     tx.AddComponent(lamp, Transform{ .Position = { 0, 2, 0 } });
	//     brick.Commit(std::move(tx));
	class Transaction
	{
	public:
		explicit Transaction(std::string label = {}, const TypeRegistry& registry = TypeRegistry::Global());

		// Shown in the undo history
		std::string Label;
		// Consecutive commits with the same non-zero key become one undo step (e.g. a slider drag)
		uint64_t MergeKey = 0;

		ObjectId CreateObject(std::string name, ObjectId parent = {}, std::string order = {});
		void CreateObject(ObjectId id, std::string name, ObjectId parent = {}, std::string order = {});
		void DestroyObject(ObjectId id);
		void DestroyTree(const BrickSnapshot& snapshot, ObjectId root); // the object and its descendants
		void Rename(ObjectId id, std::string name);
		void Move(ObjectId id, ObjectId parent, std::string order = {});

		void AddComponent(ObjectId id, TypeId type, ComponentRef value = nullptr);
		void RemoveComponent(ObjectId id, TypeId type);
		void SetField(ObjectId id, TypeId type, FieldPath path, FieldValue value);

		template<typename T>
		void AddComponent(ObjectId id, const T& value = T())
		{
			const TypeInfo* type = TypeOf<T>();
			if (type)
				AddComponent(id, type->Id, ComponentValue::MakeCopy(*type, &value));
		}

		template<typename T>
		void RemoveComponent(ObjectId id)
		{
			if (const TypeInfo* type = TypeOf<T>())
				RemoveComponent(id, type->Id);
		}

		// `field` is a dotted path such as "Bulb.Color"
		template<typename T>
		void SetField(ObjectId id, std::string_view field, FieldValue value)
		{
			const TypeInfo* type = TypeOf<T>();
			if (!type)
				return;
			auto path = ParseFieldPath(*type, field);
			if (!path)
				return Fail("unknown field " + type->Name + "." + std::string(field));
			SetField(id, type->Id, std::move(*path), std::move(value));
		}

		void Append(BrickOperation op) { m_Ops.push_back(std::move(op)); }

		const std::vector<BrickOperation>& Ops() const { return m_Ops; }
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
		std::vector<BrickOperation> m_Ops;
		std::string m_Error;
	};

}
