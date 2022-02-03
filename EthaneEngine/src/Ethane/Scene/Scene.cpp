#include "ethpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components.h"

// Renderer
#include "Ethane/Renderer/Renderer.h"
#include "Ethane/Renderer/Renderer2D.h"
#include "Ethane/Renderer/SceneRenderer.h"

#include <glm/glm.hpp>

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_fixture.h"

namespace Ethane {

	static b2BodyType Rigidbody2DTypeToB2BodyType(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody2DComponent::BodyType::Static:    return b2_staticBody;
		case Rigidbody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
		case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
		}

		ETH_CORE_ASSERT("Unknown body type");
		return b2_staticBody;
	}

	Scene::Scene()
	{
		// temp
		// m_Mesh = CreateRef<Mesh>("resources/meshes/default/Cube.fbx");
		// m_Mesh = CreateRef<Mesh>("resources/meshes/default/Sphere.fbx");
		// m_Material = CreateRef<Material>();
		Init();
	}

	Scene::~Scene()
	{
	}

	void Scene::Init()
	{
		m_Renderer2D = CreateRef<Renderer2D>();
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

	void Scene::OnRuntimeStart()
	{
		m_PhysicsWorld = new b2World({ 0.0f, -9.8f });

		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rigidbody2d = entity.GetComponent<Rigidbody2DComponent>();

			b2BodyDef bodyDef;
			bodyDef.type = Rigidbody2DTypeToB2BodyType(rigidbody2d.Type);
			bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
			bodyDef.angle = transform.Rotation.z;

			b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rigidbody2d.FixedRotation);
			rigidbody2d.RuntimeBody = body;

			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto& boxCollider2d = entity.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape b2Shape;
				b2Shape.SetAsBox(boxCollider2d.Size.x * transform.Scale.x, boxCollider2d.Size.y * transform.Scale.y);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &b2Shape;
				fixtureDef.density = boxCollider2d.Density;
				fixtureDef.friction = boxCollider2d.Friction;
				fixtureDef.restitution = boxCollider2d.Restitution;
				fixtureDef.restitutionThreshold = boxCollider2d.RestitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}
		}
	}

	void Scene::OnRuntimeStop()
	{
		delete m_PhysicsWorld;
		m_PhysicsWorld = nullptr;
	}

	void Scene::OnUpdateRuntime(Ref<SceneRenderer> renderer, Timestep ts)
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

		// Physics
		{
			const int32_t velocityIterations = 6;
			const int32_t positionIterations = 2;
			m_PhysicsWorld->Step(ts, velocityIterations, positionIterations);

			// Retrieve transform from Box2D
			auto view = m_Registry.view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& rigidbody2d = entity.GetComponent<Rigidbody2DComponent>();

				b2Body* body = (b2Body*)rigidbody2d.RuntimeBody;
				const auto& position = body->GetPosition();
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			}
		}

		// Find Primary Camera
		SceneCamera* mainCamera = nullptr;
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

		// Render
		if (mainCamera)
		{
			mainCamera->SetViewportSize(m_ViewportWidth, m_ViewportHeight);

			renderer->BeginScene(*mainCamera, cameraTransform);

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

			renderer->EndScene();
#if 0
			// Render 2D
			Renderer2D::BeginScene(mainCamera->GetProjection(), cameraTransform);
			
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
			}

			Renderer2D::EndScene();
#endif
		}
	}

	void Scene::OnUpdateEditor(Ref<SceneRenderer> renderer, Timestep ts, EditorCamera& camera)
	{
#if 1
		renderer->BeginScene(camera, camera.GetViewMatrix());

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

#if 1
		m_Renderer2D->BeginScene(camera);

		// Srpite
		{
			auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>(); //(entt::get<SpriteRendererComponent>)
			for (auto entity : view)
			{
				auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

				m_Renderer2D->DrawQuad(transform.GetTransform(), sprite.Color, (int)entity);
			}
		}
		// Texture2D
		{
			auto view = m_Registry.view<TransformComponent, Texture2DRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, texture] = view.get<TransformComponent, Texture2DRendererComponent>(entity);

				m_Renderer2D->DrawTexture(transform.GetTransform(), texture, (int)entity);
			}
		}
		// SubTexture
		{
			auto view = m_Registry.view<TransformComponent, SubTexture2DRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, texture] = view.get<TransformComponent, SubTexture2DRendererComponent>(entity);

				m_Renderer2D->DrawTexture(transform.GetTransform(), texture, (int)entity);
			}
		}

		m_Renderer2D->EndScene();
# endif
	}

	void Scene::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	// TODO: this function is not used right now
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

	template<>
	void Scene::OnComponentAdded<MeshComponent>(Entity entity, MeshComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}
}