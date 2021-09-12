#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABEL_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "SceneCamera.h"
#include "ScriptableEntity.h"

#include "Ethane/Renderer/Texture.h"
#include "Ethane/Renderer/SubTexture2D.h"
#include "Ethane/Renderer/Material.h"

namespace Ethane {

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			:Tag(tag) {}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			:Translation(translation) {}
		
		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
			
			return glm::scale(glm::translate(glm::mat4(1.0f), Translation) * rotation, Scale);
		}
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color)
			:Color(color) {}

		operator const glm::vec4& () const { return Color; }
		operator glm::vec4& () { return Color; }
	};

	struct Texture2DRendererComponent
	{
		Ref<Texture2D> Texture = Texture2D::Create("assets/textures/test.png");
		// Texture2D::Create(1, 1)
		float TilingFactor = 1.0f;
		glm::vec4 TintColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		bool test = false;

		Texture2DRendererComponent() = default;
		Texture2DRendererComponent(const Texture2DRendererComponent&) = default;
		Texture2DRendererComponent(const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
			:Texture(texture), TilingFactor(tilingFactor), TintColor(tintColor) {}
	};

	struct SubTexture2DRendererComponent
	{
		Ref<SubTexture2D> SubTexture;
		Ref<Texture2D> Texture = Texture2D::Create("assets/textures/test.png");
		glm::vec2 Coords = { 0, 0 };
		glm::vec2 CellSize = {128, 128};
		glm::vec2 SpriteSize = { 1, 1 };
		float TilingFactor = 1.0f;
		glm::vec4 TintColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		bool test = false;

		SubTexture2DRendererComponent() = default;
		SubTexture2DRendererComponent(const SubTexture2DRendererComponent&) = default;
		SubTexture2DRendererComponent(const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4 & tintColor)
			:SubTexture(subTexture), TilingFactor(tilingFactor), TintColor(tintColor) {}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity*(*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	struct MeshComponent
	{
		Ref<Mesh> Mesh;
		// Ref<Ethane::MaterialTable> MaterialTable = Ref<Hazel::MaterialTable>::Create();
		Ref<Material> Material;
		bool IsFractured = false;
	
		MeshComponent() = default;
		MeshComponent(const MeshComponent& other) = default;
		MeshComponent(const Ref<Ethane::Mesh>& mesh)
			: Mesh(mesh) {}
	
		operator Ref<Ethane::Mesh>() { return Mesh; }
	};

}
