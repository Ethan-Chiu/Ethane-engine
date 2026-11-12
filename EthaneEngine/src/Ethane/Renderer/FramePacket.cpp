#include "ethpch.h"
#include "Ethane/Renderer/FramePacket.h"

namespace Ethane {

	void FramePacket::AddMesh(AssetId mesh, const glm::mat4& transform, ElementId element, const MaterialOverride* override)
	{
		DrawItem& item = m_DrawItems.emplace_back();
		item.Mesh = mesh;
		item.Transform = transform;
		item.Element = element;
		if (override && element.IsValid())
		{
			item.Override = static_cast<int32_t>(m_Overrides.size());
			m_Overrides.push_back(*override);
		}
	}

	void FramePacket::Clear()
	{
		View = {};
		DirectionalLights.clear();
		m_DrawItems.clear();
		m_Overrides.clear();
	}

}
