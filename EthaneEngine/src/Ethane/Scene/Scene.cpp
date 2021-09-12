#include "ethpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components.h"

// Renderer
#include "Ethane/Renderer/Renderer.h"
#include "Ethane/Renderer/Renderer2D.h"
#include "Ethane/Renderer/SceneRenderer.h"

#include <glm/glm.hpp>

namespace Ethane {

	Scene::Scene()
	{
		// temp
		// m_Mesh = CreateRef<Mesh>("resources/meshes/default/Cube.fbx");
		// m_Mesh = CreateRef<Mesh>("resources/meshes/default/Sphere.fbx");
		// m_Material = CreateRef<Material>();
	}

	Scene::~Scene()
	{

	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		// Update Scripts
		{
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ entity, this };
					nsc.Instance->OnCreate();
				}

				nsc.Instance->OnUpdate(ts);
			});
		}
		// Render 2D
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transform.GetTransform();
					break;
				}
			}
		}

		if (mainCamera)
		{
			Renderer2D::BeginScene(mainCamera->GetProjection(), cameraTransform);
			
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
			}

			Renderer2D::EndScene();
		}
	}

	void Scene::OnUpdateEditor(Ref<SceneRenderer> renderer, Timestep ts, EditorCamera& camera)
	{
#if 1
		renderer->BeginScene(camera);

		auto group = m_Registry.group<MeshComponent>(entt::get<TransformComponent>);
		for (auto entity : group)
		{
			auto [meshComponent, transformComponent] = group.get<MeshComponent, TransformComponent>(entity);
			if (meshComponent.Mesh)
			{
				// glm::mat4 transform = GetTransformRelativeToParent(Entity{ entity, this });
				
				renderer->SubmitMesh(meshComponent.Mesh, transformComponent.GetTransform(), m_Material);
			}
		}
		// colliders
		{

		}

		renderer->EndScene();
#endif
#if 0
		Renderer2D::BeginScene(camera);

		// Srpite
		{
			auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>(); //(entt::get<SpriteRendererComponent>)
			for (auto entity : view)
			{
				auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color, (int)entity);
			}
		}
		// Texture2D
		{
			auto view = m_Registry.view<TransformComponent, Texture2DRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, texture] = view.get<TransformComponent, Texture2DRendererComponent>(entity);

				Renderer2D::DrawTexture(transform.GetTransform(), texture, (int)entity);
			}
		}
		// SubTexture
		{
			auto view = m_Registry.view<TransformComponent, SubTexture2DRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, texture] = view.get<TransformComponent, SubTexture2DRendererComponent>(entity);

				Renderer2D::DrawTexture(transform.GetTransform(), texture, (int)entity);
			}
		}

		Renderer2D::EndScene();
# endif

		// Renderer::RenderMesh(m_Mesh);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
			{
				cameraComponent.Camera.SetViewportSize(width, height);
			}
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			const auto& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return Entity{ entity, this };
		}
		return {};
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<Texture2DRendererComponent>(Entity entity, Texture2DRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SubTexture2DRendererComponent>(Entity entity, SubTexture2DRendererComponent& component)
	{
		component.SubTexture = SubTexture2D::CreateFromCoords(component.Texture, component.Coords, component.CellSize);
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}
}