#pragma once

#include "Ethane/Schema/Ids.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Ethane {

	class TypeRegistry;

	// The engine's built-in component types: plain structs, described in the schema by
	// RegisterEngineComponents. Any world implementation stores and interprets them.

	// Placement relative to the parent element (or the world, at the top level)
	struct TransformComponent
	{
		glm::vec3 Position{ 0.0f };
		glm::vec3 Rotation{ 0.0f }; // Euler angles in radians (pitch, yaw, roll), as glm::quat(vec3) takes them
		glm::vec3 Scale{ 1.0f };

		glm::mat4 Matrix() const;
	};

	struct MeshComponent
	{
		AssetId Mesh;
		bool Visible = true;
	};

	// Replaces the mesh's material factors (see MaterialOverride); without it the file's materials are used
	struct MaterialComponent
	{
		glm::vec4 BaseColor{ 1.0f };
		float Metallic = 0.0f;
		float Roughness = 0.8f;
		AssetId BaseColorTexture; // null: keep the mesh file's
	};

	// Shines along the element's forward axis (-Z after its world rotation)
	struct DirectionalLightComponent
	{
		glm::vec3 Color{ 1.0f };
		float Intensity = 1.0f;
	};

	// Adds the types above to `registry`; registering them again is a no-op
	void RegisterEngineComponents(TypeRegistry& registry);

	// Euler angles (as TransformComponent::Rotation) that point the forward axis (-Z) along `direction`
	glm::vec3 RotationTowards(const glm::vec3& direction);

}
