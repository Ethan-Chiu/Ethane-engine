#include "ethpch.h"
#include "Ethane/Brick/Brick.h"
#include "Ethane/Brick/OrderKey.h"

namespace Ethane {

	namespace {

		std::string Describe(ObjectId id) { return fmt::format("object {:016x}", id.Value); }

		// Builds the next state from the current one. Objects, child lists and components are copied
		// the first time this commit changes them; everything else stays shared with the old state.
		class StateEditor
		{
		public:
			StateEditor(const Detail::BrickState& current, const TypeRegistry& registry)
				: m_State(std::make_shared<Detail::BrickState>(current)), m_Registry(registry)
			{
				m_State->Version = current.Version + 1;
			}

			std::shared_ptr<Detail::BrickState> Release() { return std::move(m_State); }

			// Applies one op. On success appends the op as applied and its inverse (in apply order);
			// on failure returns why and leaves the outputs alone.
			std::string Apply(const BrickOperation& op, std::vector<BrickOperation>& applied, std::vector<BrickOperation>& inverse)
			{
				return std::visit([&](const auto& o) { return Do(o, applied, inverse); }, op);
			}

		private:
			const BrickObject* Find(ObjectId id) const
			{
				auto it = m_State->Objects.find(id);
				return it != m_State->Objects.end() ? it->second.get() : nullptr;
			}

			BrickObject& Mutable(ObjectId id)
			{
				auto owned = m_OwnedObjects.find(id);
				if (owned != m_OwnedObjects.end())
					return *owned->second;
				auto copy = std::make_shared<BrickObject>(*m_State->Objects.at(id));
				m_State->Objects[id] = copy;
				m_OwnedObjects[id] = copy;
				return *copy;
			}

			std::vector<ObjectId>& MutableChildren(ObjectId parent)
			{
				auto owned = m_OwnedChildren.find(parent);
				if (owned != m_OwnedChildren.end())
					return *owned->second;
				auto existing = m_State->Children.find(parent);
				auto copy = existing != m_State->Children.end()
					? std::make_shared<std::vector<ObjectId>>(*existing->second)
					: std::make_shared<std::vector<ObjectId>>();
				m_State->Children[parent] = copy;
				m_OwnedChildren[parent] = copy;
				return *copy;
			}

			// Copy of a component that this commit may change in place
			ComponentValue& MutableComponent(BrickObject& object, BrickComponent& component)
			{
				const std::pair key{ object.Id, component.Type };
				auto owned = m_OwnedComponents.find(key);
				if (owned != m_OwnedComponents.end())
					return *owned->second;
				auto copy = component.Value->Clone();
				component.Value = copy;
				m_OwnedComponents[key] = copy;
				return *copy;
			}

			std::span<const ObjectId> ChildrenOf(ObjectId parent) const
			{
				auto it = m_State->Children.find(parent);
				if (it == m_State->Children.end())
					return {};
				return { it->second->data(), it->second->size() };
			}

			bool SiblingLess(ObjectId a, ObjectId b) const
			{
				const BrickObject& oa = *Find(a);
				const BrickObject& ob = *Find(b);
				return oa.Order != ob.Order ? oa.Order < ob.Order : a < b;
			}

			void InsertChild(ObjectId parent, ObjectId child)
			{
				auto& children = MutableChildren(parent);
				auto at = std::lower_bound(children.begin(), children.end(), child,
					[&](ObjectId a, ObjectId b) { return SiblingLess(a, b); });
				children.insert(at, child);
			}

			void RemoveChild(ObjectId parent, ObjectId child)
			{
				auto& children = MutableChildren(parent);
				children.erase(std::find(children.begin(), children.end(), child));
				if (children.empty())
				{
					m_State->Children.erase(parent);
					m_OwnedChildren.erase(parent);
				}
			}

			// The order key to use: the given one, or one after the last current child of `parent`
			std::string ResolveOrder(const std::string& requested, ObjectId parent, ObjectId ignore) const
			{
				if (!requested.empty())
					return requested;
				auto children = ChildrenOf(parent);
				for (auto it = children.rbegin(); it != children.rend(); ++it)
					if (*it != ignore)
						return OrderKey::Between(Find(*it)->Order, {});
				return OrderKey::Between({}, {});
			}

			std::string CheckParent(ObjectId parent) const
			{
				if (parent.IsValid() && !Find(parent))
					return "parent " + Describe(parent) + " doesn't exist";
				return {};
			}

			// --- ops ---------------------------------------------------------------------------------

			std::string Do(const BrickOp::CreateObject& op, std::vector<BrickOperation>& applied, std::vector<BrickOperation>& inverse)
			{
				if (!op.Id.IsValid())
					return "can't create an object with the null id";
				if (Find(op.Id))
					return Describe(op.Id) + " already exists";
				if (auto error = CheckParent(op.Parent); !error.empty())
					return error;
				if (!OrderKey::IsValid(op.Order))
					return "invalid order key '" + op.Order + "'";

				auto object = std::make_shared<BrickObject>();
				object->Id = op.Id;
				object->Name = op.Name;
				object->Parent = op.Parent;
				object->Order = ResolveOrder(op.Order, op.Parent, {});
				m_State->Objects[op.Id] = object;
				m_OwnedObjects[op.Id] = object;
				InsertChild(op.Parent, op.Id);

				applied.push_back(BrickOp::CreateObject{ op.Id, op.Name, op.Parent, object->Order });
				inverse.push_back(BrickOp::DestroyObject{ op.Id });
				return {};
			}

			std::string Do(const BrickOp::DestroyObject& op, std::vector<BrickOperation>& applied, std::vector<BrickOperation>& inverse)
			{
				const BrickObject* object = Find(op.Id);
				if (!object)
					return Describe(op.Id) + " doesn't exist";
				if (!ChildrenOf(op.Id).empty())
					return Describe(op.Id) + " still has children";

				// Recreating it: the object first, then its components
				inverse.push_back(BrickOp::CreateObject{ object->Id, object->Name, object->Parent, object->Order });
				for (const BrickComponent& component : object->Components)
					inverse.push_back(BrickOp::AddComponent{ object->Id, component.Type, component.Value });

				RemoveChild(object->Parent, op.Id);
				m_State->Objects.erase(op.Id);
				m_OwnedObjects.erase(op.Id);
				std::erase_if(m_OwnedComponents, [&](const auto& entry) { return entry.first.first == op.Id; });
				applied.push_back(op);
				return {};
			}

			std::string Do(const BrickOp::RenameObject& op, std::vector<BrickOperation>& applied, std::vector<BrickOperation>& inverse)
			{
				if (!Find(op.Id))
					return Describe(op.Id) + " doesn't exist";
				BrickObject& object = Mutable(op.Id);
				inverse.push_back(BrickOp::RenameObject{ op.Id, object.Name });
				object.Name = op.Name;
				applied.push_back(op);
				return {};
			}

			std::string Do(const BrickOp::MoveObject& op, std::vector<BrickOperation>& applied, std::vector<BrickOperation>& inverse)
			{
				if (!Find(op.Id))
					return Describe(op.Id) + " doesn't exist";
				if (auto error = CheckParent(op.Parent); !error.empty())
					return error;
				if (!OrderKey::IsValid(op.Order))
					return "invalid order key '" + op.Order + "'";
				for (ObjectId ancestor = op.Parent; ancestor.IsValid(); ancestor = Find(ancestor)->Parent)
					if (ancestor == op.Id)
						return "moving " + Describe(op.Id) + " there would make it its own ancestor";

				const std::string order = ResolveOrder(op.Order, op.Parent, op.Id);
				BrickObject& object = Mutable(op.Id);
				inverse.push_back(BrickOp::MoveObject{ op.Id, object.Parent, object.Order });

				RemoveChild(object.Parent, op.Id);
				object.Parent = op.Parent;
				object.Order = order;
				InsertChild(op.Parent, op.Id);

				applied.push_back(BrickOp::MoveObject{ op.Id, op.Parent, order });
				return {};
			}

			std::string Do(const BrickOp::AddComponent& op, std::vector<BrickOperation>& applied, std::vector<BrickOperation>& inverse)
			{
				if (!Find(op.Id))
					return Describe(op.Id) + " doesn't exist";
				const TypeInfo* type = m_Registry.Find(op.Type);
				if (!type)
					return fmt::format("component type {:016x} isn't registered", op.Type.Value);
				if (op.Value && &op.Value->Type() != type)
					return "component value doesn't match its type " + type->Name;
				if (Find(op.Id)->FindComponent(op.Type))
					return Describe(op.Id) + " already has a " + type->Name;

				ComponentRef value = op.Value ? op.Value : ComponentRef(ComponentValue::MakeDefault(*type));
				auto& components = Mutable(op.Id).Components;
				auto at = std::lower_bound(components.begin(), components.end(), op.Type,
					[](const BrickComponent& c, TypeId t) { return c.Type < t; });
				components.insert(at, BrickComponent{ op.Type, value });

				applied.push_back(BrickOp::AddComponent{ op.Id, op.Type, value });
				inverse.push_back(BrickOp::RemoveComponent{ op.Id, op.Type });
				return {};
			}

			std::string Do(const BrickOp::RemoveComponent& op, std::vector<BrickOperation>& applied, std::vector<BrickOperation>& inverse)
			{
				if (!Find(op.Id))
					return Describe(op.Id) + " doesn't exist";
				if (!Find(op.Id)->FindComponent(op.Type))
					return Describe(op.Id) + " has no component " + TypeName(op.Type);

				auto& components = Mutable(op.Id).Components;
				auto it = std::find_if(components.begin(), components.end(), [&](const BrickComponent& c) { return c.Type == op.Type; });
				inverse.push_back(BrickOp::AddComponent{ op.Id, op.Type, it->Value });
				components.erase(it);
				m_OwnedComponents.erase({ op.Id, op.Type });

				applied.push_back(op);
				return {};
			}

			std::string Do(const BrickOp::SetField& op, std::vector<BrickOperation>& applied, std::vector<BrickOperation>& inverse)
			{
				if (!Find(op.Id))
					return Describe(op.Id) + " doesn't exist";
				if (!Find(op.Id)->FindComponent(op.Type))
					return Describe(op.Id) + " has no component " + TypeName(op.Type);

				const TypeInfo& type = *m_Registry.Find(op.Type);
				auto resolved = Resolve(type, op.Path);
				if (!resolved || resolved->Field->Type == FieldType::Struct)
					return "no field " + FormatFieldPath(type, op.Path) + " in " + type.Name;
				if (op.Value.index() != static_cast<size_t>(resolved->Field->Type))
					return fmt::format("{}.{} is a {}", type.Name, FormatFieldPath(type, op.Path), ToString(resolved->Field->Type));

				BrickObject& object = Mutable(op.Id);
				BrickComponent& component = *std::find_if(object.Components.begin(), object.Components.end(),
					[&](const BrickComponent& c) { return c.Type == op.Type; });
				ComponentValue& value = MutableComponent(object, component);
				std::byte* field = static_cast<std::byte*>(value.MutableData()) + resolved->Offset;

				inverse.push_back(BrickOp::SetField{ op.Id, op.Type, op.Path, ReadField(resolved->Field->Type, field) });
				WriteField(resolved->Field->Type, field, op.Value);
				applied.push_back(op);
				return {};
			}

			std::string TypeName(TypeId id) const
			{
				const TypeInfo* type = m_Registry.Find(id);
				return type ? type->Name : fmt::format("{:016x}", id.Value);
			}

			struct PairHash
			{
				size_t operator()(const std::pair<ObjectId, TypeId>& key) const
				{
					return std::hash<ObjectId>()(key.first) ^ (std::hash<TypeId>()(key.second) * 0x9e3779b97f4a7c15ull);
				}
			};

			std::shared_ptr<Detail::BrickState> m_State;
			const TypeRegistry& m_Registry;
			std::unordered_map<ObjectId, std::shared_ptr<BrickObject>> m_OwnedObjects;
			std::unordered_map<ObjectId, std::shared_ptr<std::vector<ObjectId>>> m_OwnedChildren;
			std::unordered_map<std::pair<ObjectId, TypeId>, std::shared_ptr<ComponentValue>, PairHash> m_OwnedComponents;
		};

	}

	// --- Brick -----------------------------------------------------------------------------------------

	Brick::Brick(const TypeRegistry& registry)
		: m_Registry(&registry), m_Listeners(std::make_shared<Detail::BrickListeners>())
	{
		auto state = std::make_shared<Detail::BrickState>();
		state->Registry = &registry;
		m_Snapshot = BrickSnapshot(std::move(state));
	}

	BrickSnapshot Brick::Read() const
	{
		std::lock_guard lock(m_SnapshotMutex);
		return m_Snapshot;
	}

	CommitResult Brick::Commit(Transaction transaction)
	{
		std::lock_guard commitLock(m_CommitMutex);
		const BrickSnapshot before = Read();

		CommitResult result;
		result.Version = before.Version();
		if (!transaction.Error().empty())
		{
			result.Error = transaction.Error();
			return result;
		}
		if (&transaction.Registry() != m_Registry)
		{
			result.Error = "the transaction was built with a different type registry";
			return result;
		}
		if (transaction.Empty())
		{
			result.Ok = true;
			return result;
		}

		StateEditor editor(*before.State(), *m_Registry);
		auto changes = std::make_shared<ChangeSet>();
		std::vector<std::vector<BrickOperation>> inverses; // per op, applied in reverse op order
		for (size_t i = 0; i < transaction.Ops().size(); i++)
		{
			std::vector<BrickOperation> inverse;
			std::string error = editor.Apply(transaction.Ops()[i], changes->Ops, inverse);
			if (!error.empty())
			{
				result.Error = fmt::format("op {}: {}", i, error);
				return result;
			}
			inverses.push_back(std::move(inverse));
		}
		for (auto it = inverses.rbegin(); it != inverses.rend(); ++it)
			std::move(it->begin(), it->end(), std::back_inserter(changes->Inverse));

		const BrickSnapshot after(editor.Release());
		{
			std::lock_guard lock(m_SnapshotMutex);
			m_Snapshot = after;
		}

		changes->FromVersion = before.Version();
		changes->ToVersion = after.Version();
		changes->Label = std::move(transaction.Label);
		changes->MergeKey = transaction.MergeKey;
		changes->After = after;

		std::vector<Listener> listeners;
		{
			std::lock_guard lock(m_Listeners->Mutex);
			for (const auto& [id, listener] : m_Listeners->List)
				listeners.push_back(listener);
		}
		for (const Listener& listener : listeners)
			listener(*changes);

		result.Ok = true;
		result.Version = after.Version();
		result.Changes = std::move(changes);
		return result;
	}

	BrickSubscription Brick::Subscribe(Listener listener)
	{
		std::lock_guard lock(m_Listeners->Mutex);
		const uint64_t id = m_Listeners->NextId++;
		m_Listeners->List.emplace_back(id, std::move(listener));
		return BrickSubscription(m_Listeners, id);
	}

	BrickSubscription& BrickSubscription::operator=(BrickSubscription&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			m_Listeners = std::move(other.m_Listeners);
			m_Id = other.m_Id;
			other.m_Listeners.reset();
		}
		return *this;
	}

	void BrickSubscription::Reset()
	{
		if (auto listeners = m_Listeners.lock())
		{
			std::lock_guard lock(listeners->Mutex);
			std::erase_if(listeners->List, [&](const auto& entry) { return entry.first == m_Id; });
		}
		m_Listeners.reset();
	}

}
