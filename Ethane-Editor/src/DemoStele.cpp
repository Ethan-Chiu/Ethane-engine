#include "DemoStele.h"

#include "Ethane.h"
#include "Ethane/Asset/AssetLibrary.h"
#include "Ethane/Renderer/Mesh.h"
#include "Ethane/Stele/Stele.h"
#include "Ethane/World/Components.h"
#include "Game/Spin.h"

namespace Ethane {

	namespace {

		// Scale and position that fit the mesh's bounds into a `size` cube resting on y = 0 at `slot`,
		// centered on x / z
		TransformComponent FitToBox(const AABB& bounds, float size, const glm::vec3& slot)
		{
			const glm::vec3 extent = bounds.Max - bounds.Min;
			const float scale = size / std::max({ extent.x, extent.y, extent.z, 1e-6f });
			const glm::vec3 anchor{ (bounds.Min.x + bounds.Max.x) * 0.5f, bounds.Min.y, (bounds.Min.z + bounds.Max.z) * 0.5f };
			return { .Position = slot - scale * anchor, .Scale = glm::vec3(scale) };
		}

	}

	void BuildDemoStele(Stele& stele, AssetLibrary& assets)
	{
		AssetRegistry& registry = assets.Registry();
		Transaction tx("Demo scene", stele.Registry());

		// Where play mode looks from: about where the editor camera starts
		const glm::vec3 eye{ 0.0f, 3.9f, 8.1f };
		const ElementId camera = tx.CreateElement("Camera");
		tx.AddComponent(camera, TransformComponent{ .Position = eye, .Rotation = RotationTowards(glm::vec3(0.0f, 0.4f, 0.0f) - eye) });
		tx.AddComponent(camera, CameraComponent{});

		const ElementId sun = tx.CreateElement("Sun");
		tx.AddComponent(sun, TransformComponent{ .Rotation = RotationTowards({ -0.4f, -0.8f, -0.6f }) });
		tx.AddComponent(sun, DirectionalLightComponent{ .Color = glm::vec3(1.0f), .Intensity = 0.9f });

		// Textures and factors straight from the file's .mtl: no material component
		const AssetId floorMesh = registry.Import("assets/models/CobblestoneFloor/CobblestoneFloor.obj");
		if (Ref<Mesh> floor = assets.LoadMesh(floorMesh)) { // needed now: its bounds place it
			const AABB& bounds = floor->GetBoundingBox();
			const float scale = 12.0f / std::max({ bounds.Max.x - bounds.Min.x, bounds.Max.z - bounds.Min.z, 1e-6f });
			const ElementId element = tx.CreateElement("Floor");
			tx.AddComponent(element, TransformComponent{ .Position = -scale * 0.5f * (bounds.Min + bounds.Max), .Scale = glm::vec3(scale) });
			tx.AddComponent(element, MeshComponent{ .Mesh = floorMesh });
		}

		struct Shape { const char* Name; glm::vec4 Color; float Metallic; float Roughness; const char* Texture; };
		const Shape shapes[] = {
			{ "Cube",     { 1.00f, 1.00f, 1.00f, 1.0f }, 0.0f, 0.6f, "assets/textures/test.png" },
			{ "Sphere",   { 0.95f, 0.70f, 0.25f, 1.0f }, 1.0f, 0.3f, nullptr }, // gold-ish metal
			{ "Cylinder", { 0.45f, 0.75f, 0.35f, 1.0f }, 0.0f, 0.9f, nullptr },
			{ "Cone",     { 0.30f, 0.65f, 0.80f, 1.0f }, 0.0f, 0.2f, nullptr },
			{ "Torus",    { 0.55f, 0.45f, 0.85f, 1.0f }, 1.0f, 0.5f, nullptr },
			{ "Capsule",  { 0.85f, 0.45f, 0.70f, 1.0f }, 0.0f, 0.4f, nullptr },
		};
		const ElementId group = tx.CreateElement("Shapes");
		tx.AddComponent(group, TransformComponent{});

		const float spacing = 1.6f;
		const float firstX = -spacing * 0.5f * static_cast<float>(std::size(shapes) - 1);
		for (size_t i = 0; i < std::size(shapes); i++)
		{
			const Shape& shape = shapes[i];
			const AssetId meshId = registry.Import(std::string("res/meshes/default/") + shape.Name + ".fbx");
			Ref<Mesh> mesh = assets.LoadMesh(meshId);
			if (!mesh) {
				continue; // logged by the library
			}
			const ElementId element = tx.CreateElement(shape.Name, group);
			tx.AddComponent(element, FitToBox(mesh->GetBoundingBox(), 1.0f, { firstX + spacing * static_cast<float>(i), 0.0f, 0.0f }));
			tx.AddComponent(element, MeshComponent{ .Mesh = meshId });
			tx.AddComponent(element, MaterialComponent{
				.BaseColor = shape.Color,
				.Metallic = shape.Metallic,
				.Roughness = shape.Roughness,
				.BaseColorTexture = shape.Texture ? registry.Import(shape.Texture) : AssetId{} });
			// Turns while playing (game code: Game/Spin.h)
			tx.AddComponent(element, SpinComponent{ .Speed = 0.5f });
			tx.AddComponent(element, NativeScriptComponent{ .Class = "Spin" });
		}

		CommitResult result = stele.Commit(std::move(tx));
		if (!result) {
			ETH_ERROR("Couldn't build the demo stele: {}", result.Error);
		}
	}

}
