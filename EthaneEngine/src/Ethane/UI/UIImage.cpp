#include "ethpch.h"
#include "UIImage.h"

#include "imgui_internal.h"

#include "Ethane/Renderer/RendererAPI.h"

// TODO: test
// #include "Platform/OopnGL/OpenGLTexture.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

namespace Ethane {

	UIImage::UIImage(uint32_t rendererID) // TODO: remove this
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			m_ImageID = reinterpret_cast<void*>(rendererID);
		}
		else
		{
			ETH_CORE_ASSERT("Only OpenGL can draw UIImage with renderID");
		}
	}

	UIImage::UIImage(Ref<Image2D> image)
	{
		// TODO: test temp change
		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			// Ref<OpenGLImage2D> glImage = std::dynamic_pointer_cast<OpenGLImage2D>(image);
			// ImGui::Image((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VulkanImage2D> finalImage = std::dynamic_pointer_cast<VulkanImage2D>(image);
			m_ImageID = ImGui_ImplVulkan_AddTexture(finalImage->GetImageInfo().Sampler, finalImage->GetImageInfo().ImageView, finalImage->GetDescriptor().imageLayout);
			m_Initialized = true;
		}
		// ETH_CORE_TRACE("Create UIImage with image: {0}", (const void*)(finalImage->GetImageInfo().Image));
		// ETH_CORE_TRACE("Create UIImage with image view: {0}", (const void*)(finalImage->GetImageInfo().ImageView));
		// ETH_CORE_TRACE("Create UIImage with image sampler: {0}", (const void*)(finalImage->GetImageInfo().Sampler));
		// ETH_CORE_TRACE("Create UIImage with image layout: {0}", (const void*)(finalImage->GetDescriptor().imageLayout));
	}

	UIImage::UIImage(Ref<Texture2D> texture)
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			// Ref<OpenGLImage2D> glImage = std::dynamic_pointer_cast<OpenGLImage2D>(image);
			// ImGui::Image((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VulkanTexture2D> vulkanTexture = std::dynamic_pointer_cast<VulkanTexture2D>(texture);
			const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetDescriptorImageInfo();
			m_ImageID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			m_Initialized = true;
		}
	}

	void UIImage::Draw(const ImVec2& size, const ImVec2& uv_min, const ImVec2& uv_max, const ImVec4& tint_col, const ImVec4& border_col)
	{
		ImGui::Image(m_ImageID, size, uv_min, uv_max, tint_col, border_col);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Image Button

	UIImageButton::UIImageButton(const Ref<Image2D>& image)
	{
		UIImageButton(nullptr, image);
	}

	UIImageButton::UIImageButton(const char* stringID, const Ref<Image2D>& image)
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			// Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
			// return ImGui::ImageButton((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
		else
		{
			Ref<VulkanImage2D> vulkanImage = std::dynamic_pointer_cast<VulkanImage2D>(image);
			const auto& imageInfo = vulkanImage->GetImageInfo();
			if (!imageInfo.ImageView)
				return;

			m_ImageID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
			m_ID = (ImGuiID)((((uint64_t)imageInfo.ImageView) >> 32) ^ (uint32_t)imageInfo.ImageView);
			if (stringID)
			{
				const ImGuiID strID = ImGui::GetID(stringID);
				m_ID = m_ID ^ strID;
			}
		}
	}

	UIImageButton::UIImageButton(const Ref<Texture2D>& texture)
	{
		UIImageButton(nullptr, texture);
	}

	UIImageButton::UIImageButton(const char* stringID, const Ref<Texture2D>& texture)
	{

		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			// Ref<OpenGLImage2D> image = texture->GetImage().As<OpenGLImage2D>();
			// return ImGui::ImageButton((ImTextureID)(size_t)image->GetRendererID(), size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
		else
		{
			Ref<VulkanTexture2D> vulkanTexture = std::dynamic_pointer_cast<VulkanTexture2D>(texture);

			// This is technically okay, could mean that GPU just hasn't created the texture yet
			if (!vulkanTexture->GetImage())
				return ;

			const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetDescriptorImageInfo();
			m_ImageID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			m_ID = (ImGuiID)((((uint64_t)imageInfo.imageView) >> 32) ^ (uint32_t)imageInfo.imageView);
			if (stringID)
			{
				const ImGuiID strID = ImGui::GetID(stringID);
				m_ID = m_ID ^ strID;
			}
		}
	}

	void UIImageButton::Draw(const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		m_Pressed = ImGui::ImageButtonEx(m_ID, m_ImageID, size, uv0, uv1, ImVec2{ (float)frame_padding, (float)frame_padding }, bg_col, tint_col);
	}
}