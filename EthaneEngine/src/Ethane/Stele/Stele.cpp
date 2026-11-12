#include "ethpch.h"
#include "Ethane/Stele/Stele.h"
#include "Ethane/Stele/OrderKey.h"
#include "Ethane/Stele/SteleData.h"

namespace Ethane {

	namespace {

		std::string Describe(ElementId id) { return fmt::format("element {:016x}", id.Value); }

		// Builds the next version's data from the current one. Elements, child lists and components are copied
		// the first time this commit changes them; everything else stays shared with the previous version.
		class DataBuilder
		{
		public:
			DataBuilder(const Detail::SteleData& current, const TypeRegistry& registry)
				: m_Data(std::make_shared<Detail::SteleData>(current)), m_Registry(registry)
			{
				m_Data->Version = current.Version + 1;
			}

			std::shared_ptr<Detail::SteleData> Release() { return std::move(m_Data); }

			// Applies one op. On success appends the op as applied and its inverse (in apply order);
			// on failure returns why and leaves the outputs alone.
			std::string Apply(const SteleOperation& op, std::vector<SteleOperation>& applied, std::vector<SteleOperation>& inverse)
			{
				return std::visit([&](const auto& o) { return Do(o, applied, inverse); }, op);
			}

		private:
			const SteleElement* Find(ElementId id) const
			{
				auto it = m_Data->Elements.find(id);
				return it != m_Data->Elements.end() ? it->second.get() : nullptr;
			}

			SteleElement& Mutable(ElementId id)
			{
				auto owned = m_OwnedElements.find(id);
				if (owned != m_OwnedElements.end())
					return *owned->second;
				auto copy = std::make_shared<SteleElement>(*m_Data->Elements.at(id));
				m_Data->Elements[id] = copy;
				m_OwnedElements[id] = copy;
				return *copy;
			}

			std::vector<ElementId>& MutableChildren(ElementId parent)
			{
				auto owned = m_OwnedChildren.find(parent);
				if (owned != m_OwnedChildren.end())
					return *owned->second;
				auto existing = m_Data->Children.find(parent);
				auto copy = existing != m_Data->Children.end()
					? std::make_shared<std::vector<ElementId>>(*existing->second)
					: std::make_shared<std::vector<ElementId>>();
				m_Data->Children[parent] = copy;
				m_OwnedChildren[parent] = copy;
				return *copy;
			}

			// Copy of a component that this commit may change in place
			ComponentValue& MutableComponent(SteleElement& element, SteleElement::Component& component)
			{
				const std::pair key{ element.Id, component.Type };
				auto owned = m_OwnedComponents.find(key);
				if (owned != m_OwnedComponents.end())
					return *owned->second;
				auto copy = component.Value->Clone();
				component.Value = copy;
				m_OwnedComponents[key] = copy;
				return *copy;
			}

			std::span<const ElementId> ChildrenOf(ElementId parent) const
			{
				auto it = m_Data->Children.find(parent);
				if (it == m_Data->Children.end())
					return {};
				return { it->second->data(), it->second->size() };
			}

			bool SiblingLess(ElementId a, ElementId b) const
			{
				const SteleElement& oa = *Find(a);
				const SteleElement& ob = *Find(b);
				return oa.Order != ob.Order ? oa.Order < ob.Order : a < b;
			}

			void InsertChild(ElementId parent, ElementId child)
			{
				auto& children = MutableChildren(parent);
				auto at = std::lower_bound(children.begin(), children.end(), child,
					[&](ElementId a, ElementId b) { return SiblingLess(a, b); });
				children.insert(at, child);
			}

			void RemoveChild(ElementId parent, ElementId child)
			{
				auto& children = MutableChildren(parent);
				children.erase(std::find(children.begin(), children.end(), child));
				if (children.empty())
				{
					m_Data->Children.erase(parent);
					m_OwnedChildren.erase(parent);
				}
			}

			// The order key to use: the given one, or one after the last current child of `parent`
			std::string ResolveOrder(const std::string& requested, ElementId parent, ElementId ignore) const
			{
				if (!requested.empty())
					return requested;
				auto children = ChildrenOf(parent);
				for (auto it = children.rbegin(); it != children.rend(); ++it)
					if (*it != ignore)
						return OrderKey::Between(Find(*it)->Order, {});
				return OrderKey::Between({}, {});
			}

			std::string CheckParent(ElementId parent) const
			{
				if (parent.IsValid() && !Find(parent))
					return "parent " + Describe(parent) + " doesn't exist";
				return {};
			}

			// --- ops ---------------------------------------------------------------------------------

			std::string Do(const SteleOp::CreateElement& op, std::vector<SteleOperation>& applied, std::vector<SteleOperation>& inverse)
			{
				if (!op.Id.IsValid())
					return "can't create an element with the null id";
				if (Find(op.Id))
					return Describe(op.Id) + " already exists";
				if (auto error = CheckParent(op.Parent); !error.empty())
					return error;
				if (!OrderKey::IsValid(op.Order))
					return "invalid order key '" + op.Order + "'";

				auto element = std::make_shared<SteleElement>();
				element->Id = op.Id;
				element->Name = op.Name;
				element->Parent = op.Parent;
				element->Order = ResolveOrder(op.Order, op.Parent, {});
				m_Data->Elements[op.Id] = element;
				m_OwnedElements[op.Id] = element;
				InsertChild(op.Parent, op.Id);

				applied.push_back(SteleOp::CreateElement{ op.Id, op.Name, op.Parent, element->Order });
				inverse.push_back(SteleOp::DestroyElement{ op.Id });
				return {};
			}

			std::string Do(const SteleOp::DestroyElement& op, std::vector<SteleOperation>& applied, std::vector<SteleOperation>& inverse)
			{
				const SteleElement* element = Find(op.Id);
				if (!element)
					return Describe(op.Id) + " doesn't exist";
				if (!ChildrenOf(op.Id).empty())
					return Describe(op.Id) + " still has children";

				// Recreating it: the element first, then its components
				inverse.push_back(SteleOp::CreateElement{ element->Id, element->Name, element->Parent, element->Order });
				for (const SteleElement::Component& component : element->Components)
					inverse.push_back(SteleOp::AddComponent{ element->Id, component.Type, component.Value });

				RemoveChild(element->Parent, op.Id);
				m_Data->Elements.erase(op.Id);
				m_OwnedElements.erase(op.Id);
				std::erase_if(m_OwnedComponents, [&](const auto& entry) { return entry.first.first == op.Id; });
				applied.push_back(op);
				return {};
			}

			std::string Do(const SteleOp::RenameElement& op, std::vector<SteleOperation>& applied, std::vector<SteleOperation>& inverse)
			{
				if (!Find(op.Id))
					return Describe(op.Id) + " doesn't exist";
				SteleElement& element = Mutable(op.Id);
				inverse.push_back(SteleOp::RenameElement{ op.Id, element.Name });
				element.Name = op.Name;
				applied.push_back(op);
				return {};
			}

			std::string Do(const SteleOp::MoveElement& op, std::vector<SteleOperation>& applied, std::vector<SteleOperation>& inverse)
			{
				if (!Find(op.Id))
					return Describe(op.Id) + " doesn't exist";
				if (auto error = CheckParent(op.Parent); !error.empty())
					return error;
				if (!OrderKey::IsValid(op.Order))
					return "invalid order key '" + op.Order + "'";
				for (ElementId ancestor = op.Parent; ancestor.IsValid(); ancestor = Find(ancestor)->Parent)
					if (ancestor == op.Id)
						return "moving " + Describe(op.Id) + " there would make it its own ancestor";

				const std::string order = ResolveOrder(op.Order, op.Parent, op.Id);
				SteleElement& element = Mutable(op.Id);
				inverse.push_back(SteleOp::MoveElement{ op.Id, element.Parent, element.Order });

				RemoveChild(element.Parent, op.Id);
				element.Parent = op.Parent;
				element.Order = order;
				InsertChild(op.Parent, op.Id);

				applied.push_back(SteleOp::MoveElement{ op.Id, op.Parent, order });
				return {};
			}

			std::string Do(const SteleOp::AddComponent& op, std::vector<SteleOperation>& applied, std::vector<SteleOperation>& inverse)
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
					[](const SteleElement::Component& c, TypeId t) { return c.Type < t; });
				components.insert(at, SteleElement::Component{ op.Type, value });

				applied.push_back(SteleOp::AddComponent{ op.Id, op.Type, value });
				inverse.push_back(SteleOp::RemoveComponent{ op.Id, op.Type });
				return {};
			}

			std::string Do(const SteleOp::RemoveComponent& op, std::vector<SteleOperation>& applied, std::vector<SteleOperation>& inverse)
			{
				if (!Find(op.Id))
					return Describe(op.Id) + " doesn't exist";
				if (!Find(op.Id)->FindComponent(op.Type))
					return Describe(op.Id) + " has no component " + TypeName(op.Type);

				auto& components = Mutable(op.Id).Components;
				auto it = std::find_if(components.begin(), components.end(), [&](const SteleElement::Component& c) { return c.Type == op.Type; });
				inverse.push_back(SteleOp::AddComponent{ op.Id, op.Type, it->Value });
				components.erase(it);
				m_OwnedComponents.erase({ op.Id, op.Type });

				applied.push_back(op);
				return {};
			}

			std::string Do(const SteleOp::SetField& op, std::vector<SteleOperation>& applied, std::vector<SteleOperation>& inverse)
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

				SteleElement& element = Mutable(op.Id);
				SteleElement::Component& component = *std::find_if(element.Components.begin(), element.Components.end(),
					[&](const SteleElement::Component& c) { return c.Type == op.Type; });
				ComponentValue& value = MutableComponent(element, component);
				std::byte* field = static_cast<std::byte*>(value.MutableData()) + resolved->Offset;

				inverse.push_back(SteleOp::SetField{ op.Id, op.Type, op.Path, ReadField(resolved->Field->Type, field) });
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
				size_t operator()(const std::pair<ElementId, TypeId>& key) const
				{
					return std::hash<ElementId>()(key.first) ^ (std::hash<TypeId>()(key.second) * 0x9e3779b97f4a7c15ull);
				}
			};

			std::shared_ptr<Detail::SteleData> m_Data;
			const TypeRegistry& m_Registry;
			std::unordered_map<ElementId, std::shared_ptr<SteleElement>> m_OwnedElements;
			std::unordered_map<ElementId, std::shared_ptr<std::vector<ElementId>>> m_OwnedChildren;
			std::unordered_map<std::pair<ElementId, TypeId>, std::shared_ptr<ComponentValue>, PairHash> m_OwnedComponents;
		};

	}

	// --- Stele -----------------------------------------------------------------------------------------

	Stele::Stele(const TypeRegistry& registry)
		: m_Registry(&registry), m_Listeners(std::make_shared<Detail::SteleListeners>())
	{
		auto data = std::make_shared<Detail::SteleData>();
		data->Registry = &registry;
		m_Snapshot = SteleSnapshot(std::move(data));
	}

	SteleSnapshot Stele::Read() const
	{
		std::lock_guard lock(m_SnapshotMutex);
		return m_Snapshot;
	}

	CommitResult Stele::Commit(Transaction transaction)
	{
		std::lock_guard commitLock(m_CommitMutex);
		const SteleSnapshot before = Read();

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

		DataBuilder builder(*before.m_Data, *m_Registry);
		auto changes = std::make_shared<ChangeSet>();
		std::vector<std::vector<SteleOperation>> inverses; // per op, applied in reverse op order
		for (size_t i = 0; i < transaction.Ops().size(); i++)
		{
			std::vector<SteleOperation> inverse;
			std::string error = builder.Apply(transaction.Ops()[i], changes->Ops, inverse);
			if (!error.empty())
			{
				result.Error = fmt::format("op {}: {}", i, error);
				return result;
			}
			inverses.push_back(std::move(inverse));
		}
		for (auto it = inverses.rbegin(); it != inverses.rend(); ++it)
			std::move(it->begin(), it->end(), std::back_inserter(changes->Inverse));

		const SteleSnapshot after(builder.Release());
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

	SteleSubscription Stele::Subscribe(Listener listener)
	{
		std::lock_guard lock(m_Listeners->Mutex);
		const uint64_t id = m_Listeners->NextId++;
		m_Listeners->List.emplace_back(id, std::move(listener));
		return SteleSubscription(m_Listeners, id);
	}

	SteleSubscription& SteleSubscription::operator=(SteleSubscription&& other) noexcept
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

	void SteleSubscription::Reset()
	{
		if (auto listeners = m_Listeners.lock())
		{
			std::lock_guard lock(listeners->Mutex);
			std::erase_if(listeners->List, [&](const auto& entry) { return entry.first == m_Id; });
		}
		m_Listeners.reset();
	}

}
