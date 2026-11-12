#pragma once

#include "Ethane/Core/Base.h"
#include "Ethane/Schema/Ids.h"

#include <glm/glm.hpp>

#include <span>
#include <vector>

namespace Ethane {

	class Mesh;
	class Material;

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

	// One mesh to draw: every node and submesh of it, placed by Transform
	struct DrawItem
	{
		Ref<Mesh> MeshRef;
		glm::mat4 Transform{ 1.0f };
		uint32_t FirstMaterial = 0; // range of the packet's materials (FramePacket::MaterialsOf)
		uint32_t MaterialCount = 0;
		ElementId Element;          // what it was extracted from (picking); null if not from a stele
	};

	// Everything the renderer needs for one view of one frame, as plain data: it knows nothing about
	// steles, worlds or ECS. Built by whatever owns the scene (a world's Extract, the demo scene), then
	// handed to Renderer::Render.
	//
	// Keep one per view and Clear() it each frame: the arrays keep their capacity.
	class FramePacket
	{
	public:
		PacketView View;
		std::vector<DirectionalLight> DirectionalLights; // the renderer currently uses the first one

		// `materials` by submesh material index (as Mesh::Draw takes them); an index past the end uses
		// the first, so a single material covers the whole mesh
		void AddMesh(const Ref<Mesh>& mesh, const glm::mat4& transform, std::span<const Ref<Material>> materials, ElementId element = {});

		std::span<const DrawItem> DrawItems() const { return m_DrawItems; }
		std::span<const Ref<Material>> MaterialsOf(const DrawItem& item) const
		{
			return std::span<const Ref<Material>>(m_Materials).subspan(item.FirstMaterial, item.MaterialCount);
		}

		void Clear();

	private:
		std::vector<DrawItem> m_DrawItems;
		std::vector<Ref<Material>> m_Materials; // all draw items' materials, one range each
	};

}
