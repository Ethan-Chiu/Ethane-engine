#pragma once

#include "Ethane/Schema/Ids.h"

#include <glm/glm.hpp>

#include <span>
#include <vector>

namespace Ethane {

	// Where the scene is seen from
	struct PacketView
	{
		glm::mat4 View{ 1.0f };
		glm::mat4 Projection{ 1.0f };
		glm::vec3 Position{ 0.0f }; // world space

		glm::mat4 ViewProjection() const { return Projection * View; }
	};

	// Light arriving from infinitely far away, e.g. the sun
	struct DirectionalLight
	{
		glm::vec3 Direction{ 0.0f, -1.0f, 0.0f }; // the way the light travels (world space)
		glm::vec3 Color{ 1.0f };
		float Intensity = 1.0f;
	};

	// Replaces the factors (and optionally the base color texture) of every material of a mesh; the
	// file's other textures (normal, metallic-roughness) stay
	struct MaterialOverride
	{
		glm::vec4 BaseColor{ 1.0f };
		float Metallic = 0.0f;
		float Roughness = 0.8f;
		AssetId BaseColorTexture; // null: keep the file's

		bool operator==(const MaterialOverride&) const = default;
	};

	// One mesh to draw: every node and submesh of it, placed by Transform
	struct DrawItem
	{
		AssetId Mesh;
		glm::mat4 Transform{ 1.0f };
		int32_t Override = -1; // index into the packet's overrides, -1: the mesh file's own materials
		ElementId Element;     // what it was extracted from (picking, per-element GPU state); may be null
	};

	// Everything the renderer needs for one view of one frame, as plain data (assets by id): it knows
	// nothing about steles, worlds or ECS, holds no GPU objects, and can be built on any thread.
	//
	// Keep one per view and Clear() it each frame: the arrays keep their capacity.
	class FramePacket
	{
	public:
		PacketView View;
		std::vector<DirectionalLight> DirectionalLights; // the renderer currently uses the first one

		// A material override needs an element id: the renderer keeps that element's materials between
		// frames. Without one the override is ignored.
		void AddMesh(AssetId mesh, const glm::mat4& transform, ElementId element = {}, const MaterialOverride* override = nullptr);

		std::span<const DrawItem> DrawItems() const { return m_DrawItems; }
		const MaterialOverride* OverrideOf(const DrawItem& item) const
		{
			return item.Override >= 0 ? &m_Overrides[item.Override] : nullptr;
		}

		void Clear();

	private:
		std::vector<DrawItem> m_DrawItems;
		std::vector<MaterialOverride> m_Overrides;
	};

}
