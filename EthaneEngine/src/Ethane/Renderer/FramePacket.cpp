#include "ethpch.h"
#include "Ethane/Renderer/FramePacket.h"

namespace Ethane {

	void FramePacket::AddMesh(AssetId mesh, const glm::mat4& transform, ElementId element, const MaterialParams* params)
	{
		DrawItem& item = m_DrawItems.emplace_back();
		item.Mesh = mesh;
		item.Transform = transform;
		item.Element = element;
		if (params && element.IsValid())
		{
			item.ParamsIndex = static_cast<int32_t>(m_Params.size());
			m_Params.push_back(*params);
		}
	}

	void FramePacket::Clear()
	{
		View = {};
		DirectionalLights.clear();
		m_DrawItems.clear();
		m_Params.clear();
	}

}
