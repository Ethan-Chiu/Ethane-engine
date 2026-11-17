#pragma once

#include "ContentBrowser/ContentBrowser.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Theme/EditorTheme.h"

#include <filesystem>
#include <functional>
#include <string>

namespace Ethane {

	class AssetLibrary;

	// Draws a ContentBrowser with ImGui as a pane: back, the roots as segments (assets | res), breadcrumbs and search over
	// a grid of tiles. Images show as thumbnails (loaded in the background); folders and other files as shapes
	// in the theme's colors, files with an extension badge. Ctrl + scroll resizes the tiles. Double-click opens a folder or a stele; meshes and textures can be dragged onto the viewport,
	// the hierarchy or an asset field (an AssetDrag payload).
	class ContentBrowserPanel
	{
	public:
		static constexpr const char* kWindowName = "Content";

		using OpenSteleFn = std::function<void(const std::filesystem::path&)>;

		ContentBrowserPanel(ContentBrowser& model, AssetLibrary& assets, const EditorTheme& theme);

		void Draw(bool* open, const OpenSteleFn& openStele);

	private:
		void DrawToolbar();
		void DrawGrid(const OpenSteleFn& openStele);
		void DrawEntry(const ContentBrowser::Entry& entry, const OpenSteleFn& openStele);
		// The entry's picture in the square at `min` with edge `size` (screen coordinates)
		void DrawIcon(const ContentBrowser::Entry& entry, const RefCountPtr<ITexture2D>& thumbnail, glm::vec2 min, float size) const;
		EditorTheme::Color KindColor(ContentBrowser::Kind kind) const;

		ContentBrowser* m_Model;
		AssetLibrary* m_Assets;
		const EditorTheme* m_Theme;

		float m_CellSize = 64.0f; // thumbnail edge, in points (Ctrl + scroll)
		std::string m_Search;
		std::filesystem::path m_Selected;
	};

}
