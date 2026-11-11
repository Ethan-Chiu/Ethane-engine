#include "ethpch.h"
#include "Ethane/Brick/Transaction.h"
#include "Ethane/Brick/BrickSnapshot.h"

namespace Ethane {

	Transaction::Transaction(std::string label, const TypeRegistry& registry)
		: Label(std::move(label)), m_Registry(&registry)
	{
	}

	ObjectId Transaction::CreateObject(std::string name, ObjectId parent, std::string order)
	{
		const ObjectId id = ObjectId::Generate();
		CreateObject(id, std::move(name), parent, std::move(order));
		return id;
	}

	void Transaction::CreateObject(ObjectId id, std::string name, ObjectId parent, std::string order)
	{
		m_Ops.push_back(BrickOp::CreateObject{ id, std::move(name), parent, std::move(order) });
	}

	void Transaction::DestroyObject(ObjectId id)
	{
		m_Ops.push_back(BrickOp::DestroyObject{ id });
	}

	void Transaction::DestroyTree(const BrickSnapshot& snapshot, ObjectId root)
	{
		// Children before their parents: an object can only be destroyed once it has no children
		for (ObjectId child : snapshot.Children(root))
			DestroyTree(snapshot, child);
		DestroyObject(root);
	}

	void Transaction::Rename(ObjectId id, std::string name)
	{
		m_Ops.push_back(BrickOp::RenameObject{ id, std::move(name) });
	}

	void Transaction::Move(ObjectId id, ObjectId parent, std::string order)
	{
		m_Ops.push_back(BrickOp::MoveObject{ id, parent, std::move(order) });
	}

	void Transaction::AddComponent(ObjectId id, TypeId type, ComponentRef value)
	{
		m_Ops.push_back(BrickOp::AddComponent{ id, type, std::move(value) });
	}

	void Transaction::RemoveComponent(ObjectId id, TypeId type)
	{
		m_Ops.push_back(BrickOp::RemoveComponent{ id, type });
	}

	void Transaction::SetField(ObjectId id, TypeId type, FieldPath path, FieldValue value)
	{
		m_Ops.push_back(BrickOp::SetField{ id, type, std::move(path), std::move(value) });
	}

	void Transaction::Fail(std::string error)
	{
		if (m_Error.empty())
			m_Error = std::move(error);
	}

}
