#include "ethpch.h"
#include "Ethane/World/Entt/EnttWorld.h"

#include "Ethane/Renderer/FramePacket.h"
#include "Ethane/Stele/Stele.h"
#include "Ethane/World/Components.h"

#include <entt.hpp>

namespace Ethane {

	namespace {

		// Every entity: which element it is and its parent's entity
		struct Node
		{
			ElementId Id;
			entt::entity Parent = entt::null;
		};

		// How to store one schema type in the registry: the typed EnTT calls behind type-erased ops
		struct Storage
		{
			const TypeInfo* Type = nullptr;
			void (*Emplace)(entt::registry&, entt::entity, const void* value) = nullptr;
			void (*Remove)(entt::registry&, entt::entity) = nullptr;
			void* (*Get)(entt::registry&, entt::entity) = nullptr;
		};

	}

	struct EnttWorld::Impl
	{
		const TypeRegistry* Types = nullptr;
		entt::registry Registry;
		std::unordered_map<ElementId, entt::entity> Entities;
		std::unordered_map<TypeId, Storage> Storages;
		uint64_t Version = 0;
		uint32_t Rebuilds = 0;

		template<typename T>
		void Bind()
		{
			const TypeInfo* type = Types->Find<T>();
			ETH_CORE_ASSERT(type, "EnttWorld: engine components aren't registered in the stele's type registry");
			if (!type)
				return;
			Storages[type->Id] = Storage{
				.Type = type,
				.Emplace = [](entt::registry& r, entt::entity e, const void* value) { r.emplace_or_replace<T>(e, *static_cast<const T*>(value)); },
				.Remove = [](entt::registry& r, entt::entity e) { r.remove<T>(e); },
				.Get = [](entt::registry& r, entt::entity e) -> void* { return r.try_get<T>(e); },
			};
		}

		entt::entity EntityOf(ElementId id) const
		{
			auto it = Entities.find(id);
			return it != Entities.end() ? it->second : entt::entity(entt::null);
		}

		void Create(ElementId id, ElementId parent)
		{
			const entt::entity entity = Registry.create();
			Registry.emplace<Node>(entity, Node{ id, EntityOf(parent) });
			Entities[id] = entity;
		}

		void AddComponent(entt::entity entity, TypeId type, const ComponentValue& value)
		{
			if (auto storage = Storages.find(type); storage != Storages.end())
				storage->second.Emplace(Registry, entity, value.Data());
		}

		void Rebuild(const SteleSnapshot& snapshot)
		{
			Registry.clear();
			Entities.clear();
			snapshot.ForEachDepthFirst([&](const SteleElement& element) {
				Create(element.Id, element.Parent);
				for (const SteleElement::Component& component : element.Components)
					AddComponent(Entities[element.Id], component.Type, *component.Value);
			});
			Version = snapshot.Version();
		}

		// False if the op doesn't fit the world's state (it then has to rebuild)
		bool ApplyOp(const SteleOperation& operation)
		{
			return std::visit([&](const auto& op) -> bool {
				using Op = std::decay_t<decltype(op)>;
				if constexpr (std::is_same_v<Op, SteleOp::CreateElement>)
				{
					if (Entities.contains(op.Id))
						return false;
					Create(op.Id, op.Parent);
					return true;
				}
				else
				{
					const entt::entity entity = EntityOf(op.Id);
					if (entity == entt::null)
						return false;

					if constexpr (std::is_same_v<Op, SteleOp::DestroyElement>)
					{
						Registry.destroy(entity);
						Entities.erase(op.Id);
					}
					else if constexpr (std::is_same_v<Op, SteleOp::RenameElement>)
					{
						// names aren't needed at runtime
					}
					else if constexpr (std::is_same_v<Op, SteleOp::MoveElement>)
					{
						Registry.get<Node>(entity).Parent = EntityOf(op.Parent);
					}
					else if constexpr (std::is_same_v<Op, SteleOp::AddComponent>)
					{
						if (!op.Value)
							return false;
						AddComponent(entity, op.Type, *op.Value);
					}
					else if constexpr (std::is_same_v<Op, SteleOp::RemoveComponent>)
					{
						if (auto storage = Storages.find(op.Type); storage != Storages.end())
							storage->second.Remove(Registry, entity);
					}
					else if constexpr (std::is_same_v<Op, SteleOp::SetField>)
					{
						auto storage = Storages.find(op.Type);
						if (storage == Storages.end())
							return true; // a type this world doesn't store
						void* component = storage->second.Get(Registry, entity);
						return component && SetField(*storage->second.Type, component, op.Path, op.Value);
					}
					return true;
				}
			}, operation);
		}

		// World matrices of all entities, parents applied
		std::unordered_map<entt::entity, glm::mat4> WorldMatrices() const
		{
			std::unordered_map<entt::entity, glm::mat4> result;
			result.reserve(Entities.size());
			auto compute = [&](auto& self, entt::entity entity) -> const glm::mat4& {
				if (auto it = result.find(entity); it != result.end())
					return it->second;
				const TransformComponent* transform = Registry.try_get<TransformComponent>(entity);
				glm::mat4 matrix = transform ? transform->Matrix() : glm::mat4(1.0f);
				const entt::entity parent = Registry.get<Node>(entity).Parent;
				if (parent != entt::null)
					matrix = self(self, parent) * matrix;
				return result.emplace(entity, matrix).first->second;
			};
			for (const auto& [id, entity] : Entities)
				compute(compute, entity);
			return result;
		}
	};

	EnttWorld::EnttWorld(const SteleSnapshot& snapshot)
		: m_Impl(std::make_unique<Impl>())
	{
		m_Impl->Types = &snapshot.Registry();
		m_Impl->Bind<TransformComponent>();
		m_Impl->Bind<MeshComponent>();
		m_Impl->Bind<MaterialComponent>();
		m_Impl->Bind<DirectionalLightComponent>();
		m_Impl->Rebuild(snapshot);
	}

	EnttWorld::~EnttWorld() = default;

	void EnttWorld::Apply(const ChangeSet& changes)
	{
		if (changes.FromVersion == m_Impl->Version)
		{
			bool ok = true;
			for (const SteleOperation& op : changes.Ops)
				if (!(ok = m_Impl->ApplyOp(op)))
					break;
			if (ok)
			{
				m_Impl->Version = changes.ToVersion;
				return;
			}
		}
		ETH_CORE_WARN("EnttWorld at version {} can't apply changes {} -> {}: rebuilding from the snapshot",
			m_Impl->Version, changes.FromVersion, changes.ToVersion);
		m_Impl->Rebuilds++;
		m_Impl->Rebuild(changes.After);
	}

	void EnttWorld::Update(const UpdateContext&)
	{
		// No systems yet: transforms are combined when extracting
	}

	void EnttWorld::Extract(FramePacket& packet) const
	{
		const entt::registry& registry = m_Impl->Registry;
		const auto world = m_Impl->WorldMatrices();

		for (auto [entity, light] : registry.view<const DirectionalLightComponent>().each())
		{
			const glm::vec3 direction = glm::mat3(world.at(entity)) * glm::vec3(0.0f, 0.0f, -1.0f);
			if (glm::dot(direction, direction) > 1e-12f)
				packet.DirectionalLights.push_back({ glm::normalize(direction), light.Color, light.Intensity });
		}

		for (auto [entity, mesh, node] : registry.view<const MeshComponent, const Node>().each())
		{
			if (!mesh.Visible || !mesh.Mesh.IsValid())
				continue;
			MaterialParams params;
			const MaterialComponent* material = registry.try_get<MaterialComponent>(entity);
			if (material)
				params = { material->BaseColor, material->Metallic, material->Roughness, material->BaseColorTexture };
			packet.AddMesh(mesh.Mesh, world.at(entity), node.Id, material ? &params : nullptr);
		}
	}

	uint64_t EnttWorld::Version() const
	{
		return m_Impl->Version;
	}

	size_t EnttWorld::EntityCount() const
	{
		return m_Impl->Entities.size();
	}

	uint32_t EnttWorld::RebuildCount() const
	{
		return m_Impl->Rebuilds;
	}

}
