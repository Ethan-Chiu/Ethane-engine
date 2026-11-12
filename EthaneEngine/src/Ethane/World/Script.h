#pragma once

#include "Ethane/Schema/TypeRegistry.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Ethane {

	// What a world gives its scripts: component access by type id. Implemented by worlds that run
	// scripts; scripts never see the world's storage (no EnTT types here).
	class ScriptHost
	{
	public:
		virtual ~ScriptHost() = default;
		virtual void* GetComponent(uint64_t entity, TypeId type) = 0;
		virtual const TypeRegistry& Types() const = 0;
	};

	// A script's handle to the element it runs on
	class ScriptEntity
	{
	public:
		ScriptEntity() = default;
		ScriptEntity(ScriptHost& host, uint64_t entity, ElementId id) : m_Host(&host), m_Entity(entity), m_Id(id) {}

		ElementId Id() const { return m_Id; }

		// The element's component of that type in the running world (any registered type), or null
		void* Get(TypeId type) const { return m_Host ? m_Host->GetComponent(m_Entity, type) : nullptr; }

		template<typename T>
		T* Get() const
		{
			const TypeInfo* type = m_Host ? m_Host->Types().Find<T>() : nullptr;
			return type ? static_cast<T*>(Get(type->Id)) : nullptr;
		}

	private:
		ScriptHost* m_Host = nullptr;
		uint64_t m_Entity = 0;
		ElementId m_Id;
	};

	// Gameplay code attached to an element by a NativeScript component; runs only in play worlds.
	// Changes it makes to components stay in the play world (the stele isn't touched).
	//
	//     class Spin : public Script {
	//         void OnUpdate(float dt) override { Get<TransformComponent>()->Rotation.y += dt; }
	//     };
	//     ScriptRegistry::Global().Register<Spin>("Spin");
	class Script
	{
	public:
		virtual ~Script() = default;

		virtual void OnCreate() {}
		virtual void OnUpdate(float deltaTime) { (void)deltaTime; }
		virtual void OnDestroy() {}

		const ScriptEntity& Entity() const { return m_Entity; }
		void Attach(const ScriptEntity& entity) { m_Entity = entity; } // by the world, before OnCreate

	protected:
		template<typename T>
		T* Get() const { return m_Entity.Get<T>(); }

	private:
		ScriptEntity m_Entity;
	};

	// Script classes by name (the name a NativeScript component stores). Register at startup.
	class ScriptRegistry
	{
	public:
		using Factory = std::function<std::unique_ptr<Script>()>;

		static ScriptRegistry& Global();

		template<typename T>
		void Register(std::string name)
		{
			static_assert(std::is_base_of_v<Script, T>, "scripts derive from Ethane::Script");
			m_Factories[std::move(name)] = [] { return std::make_unique<T>(); };
		}

		std::unique_ptr<Script> Create(std::string_view name) const; // null if not registered
		bool Contains(std::string_view name) const;
		std::vector<std::string> Names() const; // sorted

	private:
		std::unordered_map<std::string, Factory> m_Factories;
	};

}
