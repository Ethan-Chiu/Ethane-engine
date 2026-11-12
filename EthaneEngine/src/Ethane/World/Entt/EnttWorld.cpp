#include "ethpch.h"
#include "Ethane/World/Entt/EnttWorld.h"

#include "Ethane/Renderer/FramePacket.h"
#include "Ethane/Stele/Stele.h"
#include "Ethane/World/Components.h"
#include "Ethane/World/Script.h"

#include <glm/gtc/matrix_transform.hpp>

#include <entt.hpp>

namespace Ethane {

	namespace {

		// Every entity: which element it is and its parent's entity
		struct Node
		{
			ElementId Id;
			entt::entity Parent = entt::null;
		};

		// A component type the world has no C++ type for: a private copy of its value
		struct GenericComponent
		{
			std::shared_ptr<ComponentValue> Value;
		};

		// The script running on an entity (Play mode only)
		struct ScriptInstance
		{
			std::string Class;               // the class it was created for
			std::unique_ptr<Script> Instance; // null if the class isn't registered
		};

		// How one schema type is stored: the EnTT calls behind type-erased ops. Engine components are
		// stored as themselves; other types in a GenericComponent pool of their own (`pool`).
		struct Storage
		{
			const TypeInfo* Type = nullptr;
			entt::id_type Pool = 0;
			void (*Emplace)(entt::registry&, entt::id_type pool, entt::entity, const ComponentValue& value) = nullptr;
			void (*Remove)(entt::registry&, entt::id_type pool, entt::entity) = nullptr;
			void* (*Get)(entt::registry&, entt::id_type pool, entt::entity) = nullptr;
		};

		template<typename T>
		Storage TypedStorage(const TypeInfo& type)
		{
			return Storage{
				.Type = &type,
				.Emplace = [](entt::registry& r, entt::id_type, entt::entity e, const ComponentValue& value) { r.emplace_or_replace<T>(e, value.As<T>()); },
				.Remove = [](entt::registry& r, entt::id_type, entt::entity e) { r.remove<T>(e); },
				.Get = [](entt::registry& r, entt::id_type, entt::entity e) -> void* { return r.try_get<T>(e); },
			};
		}

		Storage GenericStorage(const TypeInfo& type, entt::id_type pool)
		{
			return Storage{
				.Type = &type,
				.Pool = pool,
				.Emplace = [](entt::registry& r, entt::id_type pool, entt::entity e, const ComponentValue& value) {
					r.storage<GenericComponent>(pool).emplace(e, GenericComponent{ value.Clone() });
				},
				.Remove = [](entt::registry& r, entt::id_type pool, entt::entity e) { r.storage<GenericComponent>(pool).remove(e); },
				.Get = [](entt::registry& r, entt::id_type pool, entt::entity e) -> void* {
					auto& storage = r.storage<GenericComponent>(pool);
					return storage.contains(e) ? storage.get(e).Value->MutableData() : nullptr;
				},
			};
		}

	}

	struct EnttWorld::Impl final : ScriptHost
	{
		const TypeRegistry* Schema = nullptr;
		const ScriptRegistry* Scripts = nullptr;
		WorldMode Mode = WorldMode::Edit;
		entt::registry Registry;
		std::unordered_map<ElementId, entt::entity> Entities;
		std::unordered_map<TypeId, Storage> Storages;
		std::unordered_set<std::string> MissingScripts; // classes already warned about
		uint64_t Version = 0;
		uint32_t Rebuilds = 0;

		~Impl() override { DestroyAllScripts(); }

		template<typename T>
		void Bind()
		{
			const TypeInfo* type = Schema->Find<T>();
			ETH_CORE_ASSERT(type, "EnttWorld: engine components aren't registered in the stele's type registry");
			if (type)
				Storages[type->Id] = TypedStorage<T>(*type);
		}

		// How to store `type`: bound engine types, else a generic pool (created on first use)
		const Storage* StorageFor(TypeId type)
		{
			if (auto it = Storages.find(type); it != Storages.end())
				return &it->second;
			const TypeInfo* info = Schema->Find(type);
			if (!info)
				return nullptr;
			// EnTT pool ids are 32-bit: fold the 64-bit TypeId, and make sure two types never share a pool
			entt::id_type pool = static_cast<entt::id_type>(type.Value ^ (type.Value >> 32));
			auto taken = [&](entt::id_type id) {
				return std::any_of(Storages.begin(), Storages.end(), [&](const auto& entry) { return entry.second.Pool == id; });
			};
			while (pool == 0 || taken(pool))
				pool++;
			return &(Storages[type] = GenericStorage(*info, pool));
		}

		// ScriptHost
		void* GetComponent(uint64_t entity, TypeId type) override
		{
			const entt::entity e = static_cast<entt::entity>(entity);
			const Storage* storage = Registry.valid(e) ? StorageFor(type) : nullptr;
			return storage ? storage->Get(Registry, storage->Pool, e) : nullptr;
		}
		const TypeRegistry& Types() const override { return *Schema; }

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
			if (const Storage* storage = StorageFor(type))
				storage->Emplace(Registry, storage->Pool, entity, value);
		}

		void DestroyScript(entt::entity entity)
		{
			if (ScriptInstance* script = Registry.try_get<ScriptInstance>(entity))
			{
				if (script->Instance)
					script->Instance->OnDestroy();
				Registry.remove<ScriptInstance>(entity);
			}
		}

		void DestroyAllScripts()
		{
			for (auto [entity, script] : Registry.view<ScriptInstance>().each())
				if (script.Instance)
					script.Instance->OnDestroy();
			Registry.clear<ScriptInstance>();
		}

		void Rebuild(const SteleSnapshot& snapshot)
		{
			DestroyAllScripts(); // recreated by the next Update
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
						DestroyScript(entity);
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
						if (const Storage* storage = StorageFor(op.Type))
							storage->Remove(Registry, storage->Pool, entity);
					}
					else if constexpr (std::is_same_v<Op, SteleOp::SetField>)
					{
						const Storage* storage = StorageFor(op.Type);
						void* component = storage ? storage->Get(Registry, storage->Pool, entity) : nullptr;
						return component && SetField(*storage->Type, component, op.Path, op.Value);
					}
					return true;
				}
			}, operation);
		}

		void UpdateScripts(float deltaTime)
		{
			// Scripts whose component went away
			std::vector<entt::entity> stale;
			for (auto [entity, script] : Registry.view<ScriptInstance>().each())
			{
				const NativeScriptComponent* component = Registry.try_get<NativeScriptComponent>(entity);
				if (!component || component->Class != script.Class)
					stale.push_back(entity);
			}
			for (entt::entity entity : stale)
				DestroyScript(entity);

			// New ones: attached and created before anything updates
			std::vector<Script*> created;
			for (auto [entity, component, node] : Registry.view<NativeScriptComponent, Node>().each())
			{
				if (Registry.all_of<ScriptInstance>(entity) || component.Class.empty())
					continue;
				ScriptInstance& script = Registry.emplace<ScriptInstance>(entity, ScriptInstance{ component.Class, Scripts->Create(component.Class) });
				if (!script.Instance)
				{
					if (MissingScripts.insert(component.Class).second)
						ETH_CORE_WARN("No script class '{}' is registered", component.Class);
					continue;
				}
				script.Instance->Attach(ScriptEntity(*this, static_cast<uint64_t>(entity), node.Id));
				created.push_back(script.Instance.get());
			}
			for (Script* script : created)
				script->OnCreate();

			for (auto [entity, script] : Registry.view<ScriptInstance>().each())
				if (script.Instance)
					script.Instance->OnUpdate(deltaTime);
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

	EnttWorld::EnttWorld(const SteleSnapshot& snapshot, WorldMode mode, const ScriptRegistry* scripts)
		: m_Impl(std::make_unique<Impl>())
	{
		m_Impl->Schema = &snapshot.Registry();
		m_Impl->Scripts = scripts ? scripts : &ScriptRegistry::Global();
		m_Impl->Mode = mode;
		m_Impl->Bind<TransformComponent>();
		m_Impl->Bind<MeshComponent>();
		m_Impl->Bind<MaterialComponent>();
		m_Impl->Bind<DirectionalLightComponent>();
		m_Impl->Bind<CameraComponent>();
		m_Impl->Bind<NativeScriptComponent>();
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

	void EnttWorld::Update(const UpdateContext& context)
	{
		if (m_Impl->Mode == WorldMode::Play)
			m_Impl->UpdateScripts(context.DeltaTime);
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

	bool EnttWorld::ExtractView(float aspectRatio, PacketView& view) const
	{
		const entt::registry& registry = m_Impl->Registry;
		entt::entity chosen = entt::null;
		for (auto [entity, camera] : registry.view<const CameraComponent>().each())
		{
			if (camera.Primary)
			{
				chosen = entity;
				break;
			}
		}
		if (chosen == entt::null)
			return false;

		// The camera's placement without scale: position and orthonormal axes of its world matrix
		const glm::mat4 world = m_Impl->WorldMatrices().at(chosen);
		glm::mat4 placement(1.0f);
		for (int axis = 0; axis < 3; axis++)
		{
			const glm::vec3 column(world[axis]);
			placement[axis] = glm::vec4(glm::length(column) > 1e-6f ? glm::normalize(column) : glm::vec3(0.0f), 0.0f);
		}
		placement[3] = world[3];

		const CameraComponent& camera = registry.get<CameraComponent>(chosen);
		view.View = glm::inverse(placement);
		view.Projection = glm::perspective(camera.FieldOfView, std::max(aspectRatio, 1e-4f), std::max(camera.Near, 1e-4f), std::max(camera.Far, camera.Near + 1e-3f));
		view.Position = glm::vec3(world[3]);
		return true;
	}

	uint64_t EnttWorld::Version() const
	{
		return m_Impl->Version;
	}

	WorldMode EnttWorld::Mode() const
	{
		return m_Impl->Mode;
	}

	size_t EnttWorld::EntityCount() const
	{
		return m_Impl->Entities.size();
	}

	size_t EnttWorld::ScriptCount() const
	{
		size_t count = 0;
		for (auto [entity, script] : m_Impl->Registry.view<const ScriptInstance>().each())
			count += script.Instance ? 1 : 0;
		return count;
	}

	uint32_t EnttWorld::RebuildCount() const
	{
		return m_Impl->Rebuilds;
	}

}
