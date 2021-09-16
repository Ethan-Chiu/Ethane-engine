#pragma once

#include <filesystem>
#include "Ethane/Renderer/Texture.h"

// TODO: test
#include "Ethane/UI/UIImage.h"

namespace Ethane {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();
	private:
		std::filesystem::path m_CurrentDirectory;
		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;

		float m_Padding = 16.0f;
		float m_ThumbnailSize = 128;
	};
}