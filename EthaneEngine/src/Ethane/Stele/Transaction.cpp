#include "ethpch.h"
#include "Ethane/Stele/Transaction.h"
#include "Ethane/Stele/SteleSnapshot.h"

namespace Ethane {

	Transaction::Transaction(std::string label, const TypeRegistry& registry)
		: Label(std::move(label)), m_Registry(&registry)
	{
	}

	ElementId Transaction::CreateElement(std::string name, ElementId parent, std::string order)
	{
		const ElementId id = ElementId::Generate();
		CreateElement(id, std::move(name), parent, std::move(order));
		return id;
	}

	void Transaction::CreateElement(ElementId id, std::string name, ElementId parent, std::string order)
	{
		m_Ops.push_back(SteleOp::CreateElement{ id, std::move(name), parent, std::move(order) });
	}

	void Transaction::DestroyElement(ElementId id)
	{
		m_Ops.push_back(SteleOp::DestroyElement{ id });
	}

	void Transaction::DestroyTree(const SteleSnapshot& snapshot, ElementId root)
	{
		// Children before their parents: an element can only be destroyed once it has no children
		for (ElementId child : snapshot.Children(root))
			DestroyTree(snapshot, child);
		DestroyElement(root);
	}

	void Transaction::Rename(ElementId id, std::string name)
	{
		m_Ops.push_back(SteleOp::RenameElement{ id, std::move(name) });
	}

	void Transaction::Move(ElementId id, ElementId parent, std::string order)
	{
		m_Ops.push_back(SteleOp::MoveElement{ id, parent, std::move(order) });
	}

	void Transaction::AddComponent(ElementId id, TypeId type, ComponentRef value)
	{
		m_Ops.push_back(SteleOp::AddComponent{ id, type, std::move(value) });
	}

	void Transaction::RemoveComponent(ElementId id, TypeId type)
	{
		m_Ops.push_back(SteleOp::RemoveComponent{ id, type });
	}

	void Transaction::SetField(ElementId id, TypeId type, FieldPath path, FieldValue value)
	{
		m_Ops.push_back(SteleOp::SetField{ id, type, std::move(path), std::move(value) });
	}

	void Transaction::Fail(std::string error)
	{
		if (m_Error.empty())
			m_Error = std::move(error);
	}

}
