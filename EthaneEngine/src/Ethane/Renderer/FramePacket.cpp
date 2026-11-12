#include "ethpch.h"
#include "Ethane/Renderer/FramePacket.h"

namespace Ethane {

	void FramePacket::AddMesh(const Ref<Mesh>& mesh, const glm::mat4& transform, std::span<const Ref<Material>> materials, ElementId element)
	{
		DrawItem& item = m_DrawItems.emplace_back();
		item.MeshRef = mesh;
		item.Transform = transform;
		item.FirstMaterial = static_cast<uint32_t>(m_Materials.size());
		item.MaterialCount = static_cast<uint32_t>(materials.size());
		item.Element = element;
		m_Materials.insert(m_Materials.end(), materials.begin(), materials.end());
	}

	void FramePacket::Clear()
	{
		View = {};
		DirectionalLights.clear();
		m_DrawItems.clear();
		m_Materials.clear();
	}

}
