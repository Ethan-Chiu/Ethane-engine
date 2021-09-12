#include "ethpch.h"
#include "UIImage.h"

// TODO: test
#include "Platform/Vulkan/VulkanTexture.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

namespace Ethane {

	UIImage::UIImage(Ref<Image2D> image)
	{
		// TODO: test temp change
		Ref<VulkanImage2D> finalImage = std::dynamic_pointer_cast<VulkanImage2D>(image);
		ETH_CORE_TRACE("Create UIImage with image: {0}", (const void*)(finalImage->GetImageInfo().Image));
		ETH_CORE_TRACE("Create UIImage with image view: {0}", (const void*)(finalImage->GetImageInfo().ImageView));
		ETH_CORE_TRACE("Create UIImage with image sampler: {0}", (const void*)(finalImage->GetImageInfo().Sampler));
		ETH_CORE_TRACE("Create UIImage with image layout: {0}", (const void*)(finalImage->GetDescriptor().imageLayout));
		m_ImageID = ImGui_ImplVulkan_AddTexture(finalImage->GetImageInfo().Sampler, finalImage->GetImageInfo().ImageView, finalImage->GetDescriptor().imageLayout);
		m_Initialized = true;
	}

	void UIImage::Draw(ImVec2 size, ImVec2 uv_min, ImVec2 uv_max, ImVec4 tint_col, ImVec4 border_col)
	{
		ImGui::Image(m_ImageID, size, uv_min, uv_max, tint_col, border_col);
	}

}