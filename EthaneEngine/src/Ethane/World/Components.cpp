#include "ethpch.h"
#include "Ethane/World/Components.h"

#include "Ethane/Schema/TypeRegistry.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Ethane {

	glm::mat4 TransformComponent::Matrix() const
	{
		return glm::translate(glm::mat4(1.0f), Position) * glm::mat4_cast(glm::quat(Rotation)) * glm::scale(glm::mat4(1.0f), Scale);
	}

	void RegisterEngineComponents(TypeRegistry& registry)
	{
		if (registry.Find<TransformComponent>())
			return;

		registry.Register<TransformComponent>("Transform")
			.Field("Position", &TransformComponent::Position, FieldFlags::None, { .Speed = 0.05f })
			.Field("Rotation", &TransformComponent::Rotation, FieldFlags::Angle)
			.Field("Scale", &TransformComponent::Scale, FieldFlags::None, { .Speed = 0.01f });

		registry.Register<MeshComponent>("Mesh")
			.Field("Mesh", &MeshComponent::Mesh)
			.Field("Visible", &MeshComponent::Visible);

		registry.Register<MaterialComponent>("Material")
			.Field("BaseColor", &MaterialComponent::BaseColor, FieldFlags::Color)
			.Field("Metallic", &MaterialComponent::Metallic, FieldFlags::None, { .Min = 0.0f, .Max = 1.0f })
			.Field("Roughness", &MaterialComponent::Roughness, FieldFlags::None, { .Min = 0.0f, .Max = 1.0f })
			.Field("BaseColorTexture", &MaterialComponent::BaseColorTexture);

		registry.Register<DirectionalLightComponent>("DirectionalLight")
			.Field("Color", &DirectionalLightComponent::Color, FieldFlags::Color)
			.Field("Intensity", &DirectionalLightComponent::Intensity, FieldFlags::None, { .Min = 0.0f, .Max = 10.0f });

		registry.Register<CameraComponent>("Camera")
			.Field("FieldOfView", &CameraComponent::FieldOfView, FieldFlags::Angle)
			.Field("Near", &CameraComponent::Near, FieldFlags::None, { .Speed = 0.01f })
			.Field("Far", &CameraComponent::Far, FieldFlags::None, { .Speed = 1.0f })
			.Field("Primary", &CameraComponent::Primary);

		registry.Register<NativeScriptComponent>("NativeScript")
			.Field("Class", &NativeScriptComponent::Class);
	}

	glm::vec3 RotationTowards(const glm::vec3& direction)
	{
		// glm::quat(vec3(pitch, yaw, roll)) rotates by Rz(roll) * Ry(yaw) * Rx(pitch); with no roll the
		// forward axis (0, 0, -1) becomes (-cos(pitch) sin(yaw), sin(pitch), -cos(pitch) cos(yaw)).
		// Solved with atan2 rather than through glm::eulerAngles, which loses precision near yaw = +-90 degrees.
		const glm::vec3 d = glm::normalize(direction);
		const float pitch = std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z));
		const float yaw = std::atan2(-d.x, -d.z);
		return { pitch, yaw, 0.0f };
	}

}
