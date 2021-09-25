#include "ethpch.h"
#include "MaterialPanel.h"

namespace Ethane {

	MaterialPanel::MaterialPanel()
	{
		
	}

	MaterialPanel::MaterialPanel(Ref<Material> material)
	{
		OpenMaterialAsset(material);
	}

	void MaterialPanel::OpenMaterialAsset(Ref<Material> material)
	{
		m_Material = material;
	}

	void MaterialPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (m_Material)
		{

		}

		ImGui::End();
	}

}