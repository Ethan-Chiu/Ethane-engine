#include "ethpch.h"

#include "ContentBrowserPanel.h"
#include <imgui/imgui.h>


namespace Ethane{
	
	static std::filesystem::path s_AssetsPath = "assets";

	ContentBrowserPanel::ContentBrowserPanel()
		: m_CurrentDirectory(s_AssetsPath)
	{
		m_DirectoryIcon = Texture2D::Create("resources/icons/ContentBrowser/folder.png");
		m_FileIcon = Texture2D::Create("resources/icons/ContentBrowser/file.png");
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");
		
		if (m_CurrentDirectory != std::filesystem::path(s_AssetsPath))
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		float cellSize = m_ThumbnailSize + m_Padding;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, 0, false);
			
		for (auto& currentPath : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& path = currentPath.path();
			std::string pathString = path.string();
			std::string filenameString = std::filesystem::relative(path, s_AssetsPath).filename().string();
			
			if (currentPath.is_directory())
			{
				// ImGui::ImageButton((ImTextureID)m_DirectoryIcon->GetRendererID(), { m_ThumbnailSize, m_ThumbnailSize }, { 0, 1 }, { 1, 0 });
				UIImageButton(filenameString.c_str(), m_DirectoryIcon).Draw({ m_ThumbnailSize, m_ThumbnailSize });
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_CurrentDirectory /= path.filename();
				}
			}
			else
			{
				// ImGui::ImageButton((ImTextureID)m_FileIcon->GetRendererID(), { m_ThumbnailSize, m_ThumbnailSize }, { 0, 1 }, { 1, 0 });
				UIImageButton(filenameString.c_str(), m_FileIcon).Draw({ m_ThumbnailSize, m_ThumbnailSize });
			}
			ImGui::Text(filenameString.c_str());

			ImGui::NextColumn();
		}
		ImGui::Columns(1);

		ImGui::SliderFloat("Thumbnail Size", &m_ThumbnailSize, 16, 512);
		ImGui::SameLine();
		ImGui::SliderFloat("Padding", &m_Padding, 0, 64);

		ImGui::End();
	}
}