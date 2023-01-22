#pragma once

#include "Ethane/Renderer/Image.h"
#include "Ethane/Renderer/Texture.h"

#include "imgui.h"

namespace Ethane {

	class UIImage {

	public:
		UIImage() = default;
		//UIImage(Ref<Image2D> image);
		UIImage(Texture2D* texture);
		UIImage(uint32_t rendererID); // TODO: remove this maybe
		~UIImage() = default;

		void Draw(const ImVec2& size, const ImVec2& uv_min = { 0.0f, 0.0f }, const ImVec2& uv_max = { 1.0f, 1.0f }, const ImVec4& tint_col = { 1.0f, 1.0f, 1.0f, 1.0f }, const ImVec4& border_col = { 1.0f, 1.0f, 1.0f, 0.5f });

		operator bool() const { return m_Initialized; }

		ImTextureID GetTextureID() { return m_ImageID; }
	private:
		bool m_Initialized = false;
		ImTextureID m_ImageID = nullptr;
	};

	class UIImageButton {

	public:
		UIImageButton() = default;
		//UIImageButton(const Ref<Image2D>& image);
		//UIImageButton(const char* stringID, const Ref<Image2D>& image);
		UIImageButton(const Ref<Texture2D>& texture);
		UIImageButton(const char* stringID, const Ref<Texture2D>& texture);
		~UIImageButton() = default;

		void Draw(const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
		bool Pressed() { return m_Pressed; }

		operator bool() const { return m_Initialized; }
	private:
		bool m_Initialized = false;
		ImTextureID m_ImageID = nullptr;
		ImGuiID m_ID;
		bool m_Pressed = false;
	};

}