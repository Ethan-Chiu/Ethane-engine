#pragma once

#include "entt.hpp"

#include "Ethane/Core/Timestep.h"
#include "Ethane/Renderer/EditorCamera.h"

#include "Ethane/Renderer/Mesh.h"
#include "Ethane/Renderer/Material.h"

namespace Ethane {

	class SceneRenderer;
	class Entity;

	class Scene {
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnUpdateEditor(Ref<SceneRenderer> renderer, Timestep ts, EditorCamera& camera);
		void OnUpdateRuntime(Timestep ts);

		void OnViewportResize(uint32_t width, uint32_t height);

		Entity GetPrimaryCameraEntity();
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
	private:
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		// TODO: remove
		Ref<Mesh> m_Mesh;
		Ref<Material> m_Material;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}