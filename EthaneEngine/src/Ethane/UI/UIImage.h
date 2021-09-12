#pragma once

#include "Ethane/Renderer/Image.h"

#include "imgui.h"

namespace Ethane {

	class UIImage {

	public:
		UIImage() = default;
		UIImage(Ref<Image2D> image);
		~UIImage() = default;

		void Draw(ImVec2 size, ImVec2 uv_min = { 0.0f, 0.0f }, ImVec2 uv_max = { 1.0f, 1.0f }, ImVec4 tint_col = { 1.0f, 1.0f, 1.0f, 1.0f }, ImVec4 border_col = { 1.0f, 1.0f, 1.0f, 0.5f });

		operator bool() const { return m_Initialized; }
	private:
		// TODO: test
		ImTextureID m_ImageID = nullptr;
		bool m_Initialized = false;
	};

}