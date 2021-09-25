#pragma once

#include <filesystem>
#include "Ethane/Renderer/Material.h"

// TODO: test
#include "Ethane/UI/UIImage.h"

namespace Ethane {

	class MaterialPanel
	{
	public:
		MaterialPanel();
		MaterialPanel(Ref<Material> material);

		void OnImGuiRender();
	private:
		void OpenMaterialAsset(Ref<Material> material);
	private:
		Ref<Material> m_Material;
	};
}