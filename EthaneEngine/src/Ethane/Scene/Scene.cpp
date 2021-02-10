#include "ethpch.h"
#include "Scene.h"

#include "Components.h"
#include "Ethane/Renderer/Renderer2D.h"
#include <glm/glm.hpp>

namespace Ethane {

	static void DoMath(const glm::mat4& tarnsform)
	{

	}

	Scene::Scene()
	{
		// struct TransformComponent
		// {
		// 	glm::mat4 Transform;
		// 
		// 	TransformComponent() = default;
		// 	TransformComponent(const TransformComponent&) = default;
		// 	TransformComponent(const glm::mat4& transform)
		// 		:Transform(transform) {}
		// 
		// 	operator const glm::mat4& () const { return Transform; }
		// 	operator glm::mat4& () { return Transform; }
		// };
		// 
		// entt::entity entity = m_Registry.create();
		// m_Registry.emplace< TransformComponent>(entity, glm::mat4(1.0f));
	}

	Scene::~Scene()
	{

	}

	entt::entity Scene::CreateEntity()
	{
		return m_Registry.create();
	}

	void Scene::OnUpdate(Timestep ts)
	{
		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entity : group)
		{
			auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

			Renderer2D::DrawQuad(transform, sprite.Color);
		}
	}
}