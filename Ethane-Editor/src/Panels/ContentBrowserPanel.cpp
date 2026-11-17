#include "ContentBrowserPanel.h"

#include "ContentBrowser/AssetDrag.h"
#include "Ethane/Asset/AssetLibrary.h"
#include "Ethane/Core/Application.h"
#include "Theme/ImGuiTheme.h"

#include <imgui.h>

#include <cctype>
#include <misc/cpp/imgui_stdlib.h>

namespace Ethane {

	namespace {

		ImTextureID TextureId(const RefCountPtr<ITexture2D>& texture)
		{
			return Application::Get().GetImGuiLayer()->GetTextureID(texture);
		}

		// `text` shortened with "..." to fit `width`
		std::string Elide(const std::string& text, float width)
		{
			if (ImGui::CalcTextSize(text.c_str()).x <= width)
				return text;
			std::string shortened = text;
			while (!shortened.empty() && ImGui::CalcTextSize((shortened + "...").c_str()).x > width)
				shortened.pop_back();
			return shortened + "...";
		}

		std::string DisplayName(const std::filesystem::path& folder)
		{
			const std::string name = folder.filename().string();
			return name.empty() ? folder.generic_string() : name;
		}

	}

	ContentBrowserPanel::ContentBrowserPanel(ContentBrowser& model, AssetLibrary& assets, const EditorTheme& theme)
		: m_Model(&model), m_Assets(&assets), m_Theme(&theme)
	{
	}

	void ContentBrowserPanel::Draw(bool* open, const OpenSteleFn& openStele)
	{
		ImGui::Begin(kWindowName, open);
		DrawToolbar();
		ImGui::Separator();
		DrawGrid(openStele);
		ImGui::End();
	}

	void ContentBrowserPanel::DrawToolbar()
	{
		const EditorTheme& theme = *m_Theme;

		// Root
		if (m_Model->Roots().size() > 1)
		{
			ImGui::SetNextItemWidth(ImGui::CalcTextSize(m_Model->Root().generic_string().c_str()).x + ImGui::GetFrameHeight() + 16.0f);
			if (ImGui::BeginCombo("##root", m_Model->Root().generic_string().c_str()))
			{
				for (const std::filesystem::path& root : m_Model->Roots())
					if (ImGui::Selectable(root.generic_string().c_str(), root == m_Model->Root()))
						m_Model->Navigate(root);
				ImGui::EndCombo();
			}
			ImGui::SameLine();
		}

		// Back, then the path as buttons
		ImGui::BeginDisabled(!m_Model->CanGoUp());
		if (ImGui::ArrowButton("##up", ImGuiDir_Left))
			m_Model->GoUp();
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Up a folder");

		ImGui::PushStyleColor(ImGuiCol_Button, ToImGui(EditorTheme::WithAlpha(theme.Surface, 0.0f)));
		const auto crumbs = m_Model->Breadcrumbs();
		for (size_t i = 0; i < crumbs.size(); i++)
		{
			ImGui::SameLine(0.0f, i == 0 ? -1.0f : 2.0f);
			if (i > 0)
			{
				ImGui::TextColored(ToImGui(theme.TextMuted), "/");
				ImGui::SameLine(0.0f, 2.0f);
			}
			ImGui::PushID(static_cast<int>(i));
			const bool last = i + 1 == crumbs.size();
			if (!last)
				ImGui::PushStyleColor(ImGuiCol_Text, ToImGui(theme.TextMuted));
			if (ImGui::Button(DisplayName(crumbs[i]).c_str()))
				m_Model->Navigate(crumbs[i]);
			if (!last)
				ImGui::PopStyleColor();
			ImGui::PopID();
		}
		ImGui::PopStyleColor();

		// Right side: search and the thumbnail size
		const float searchWidth = 180.0f, sizeWidth = 80.0f;
		const float right = ImGui::GetWindowContentRegionMax().x;
		ImGui::SameLine(std::max(ImGui::GetCursorPosX(), right - searchWidth - sizeWidth - ImGui::GetStyle().ItemSpacing.x));
		ImGui::SetNextItemWidth(searchWidth);
		if (ImGui::InputTextWithHint("##search", "Search", &m_Search))
			m_Model->SetFilter(m_Search);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(sizeWidth);
		ImGui::SliderFloat("##size", &m_CellSize, 48.0f, 160.0f, "");
		ImGui::SetItemTooltip("Thumbnail size");
	}

	void ContentBrowserPanel::DrawGrid(const OpenSteleFn& openStele)
	{
		ImGui::BeginChild("##grid");
		const auto entries = m_Model->Entries();
		if (entries.empty())
			ImGui::TextDisabled(m_Model->Filter().empty() ? "This folder is empty" : "No matches");

		const ImGuiStyle& style = ImGui::GetStyle();
		const float cellWidth = m_CellSize + style.FramePadding.x * 2.0f;
		const float available = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(1, static_cast<int>((available + style.ItemSpacing.x) / (cellWidth + style.ItemSpacing.x)));
		for (size_t i = 0; i < entries.size(); i++)
		{
			if (i % columns != 0)
				ImGui::SameLine();
			// A copy: opening a folder rescans, which replaces the entries
			const ContentBrowser::Entry entry = entries[i];
			DrawEntry(entry, openStele);
			if (m_Model->Entries().data() != entries.data())
				break; // navigated: the rest belongs to the old folder
		}

		if (ImGui::BeginPopupContextWindow("##grid", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Refresh"))
				m_Model->Refresh();
			ImGui::EndPopup();
		}
		ImGui::EndChild();
	}

	EditorTheme::Color ContentBrowserPanel::KindColor(ContentBrowser::Kind kind) const
	{
		switch (kind)
		{
			case ContentBrowser::Kind::Folder: return m_Theme->Secondary;
			case ContentBrowser::Kind::Mesh: return m_Theme->Accent;
			case ContentBrowser::Kind::Texture: return m_Theme->Text;
			case ContentBrowser::Kind::Stele: return m_Theme->Danger;
			case ContentBrowser::Kind::Other: break;
		}
		return m_Theme->TextMuted;
	}

	EditorTheme::Color ContentBrowserPanel::BandText(const EditorTheme::Color& band) const
	{
		const EditorTheme& theme = *m_Theme;
		const bool textIsDark = EditorTheme::Luminance(theme.Text) < EditorTheme::Luminance(theme.Background);
		const EditorTheme::Color& dark = textIsDark ? theme.Text : theme.Background;
		const EditorTheme::Color& light = textIsDark ? theme.Background : theme.Text;
		return EditorTheme::Luminance(band) > 0.55f ? dark : light;
	}

	void ContentBrowserPanel::DrawIcon(const ContentBrowser::Entry& entry, const RefCountPtr<ITexture2D>& thumbnail, glm::vec2 min, float size) const
	{
		const EditorTheme& theme = *m_Theme;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		auto at = [&](float x, float y) { return ImVec2(min.x + x * size, min.y + y * size); };
		const float rounding = theme.Rounding;

		if (thumbnail)
		{
			draw->AddImageRounded(TextureId(thumbnail), at(0.08f, 0.08f), at(0.92f, 0.92f), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, rounding);
			return;
		}

		const EditorTheme::Color color = KindColor(entry.Type);
		if (entry.Type == ContentBrowser::Kind::Folder)
		{
			// A tab, then the body over it, a little lighter at the front
			draw->AddRectFilled(at(0.12f, 0.2f), at(0.48f, 0.36f), ToImGuiU32(EditorTheme::WithAlpha(color, 0.7f)), rounding);
			draw->AddRectFilled(at(0.12f, 0.28f), at(0.88f, 0.8f), ToImGuiU32(EditorTheme::WithAlpha(color, 0.7f)), rounding);
			draw->AddRectFilled(at(0.12f, 0.36f), at(0.88f, 0.8f), ToImGuiU32(color), rounding);
			return;
		}

		// A page with a folded corner and a band in the kind's color carrying the extension
		const float fold = 0.16f;
		const ImU32 page = ToImGuiU32(theme.Surface), edge = ToImGuiU32(theme.Border);
		const ImVec2 outline[] = { at(0.2f, 0.1f), at(0.8f - fold, 0.1f), at(0.8f, 0.1f + fold), at(0.8f, 0.9f), at(0.2f, 0.9f) };
		draw->AddConvexPolyFilled(outline, 5, page);
		draw->AddPolyline(outline, 5, edge, ImDrawFlags_Closed, 1.0f);
		draw->AddTriangleFilled(at(0.8f - fold, 0.1f), at(0.8f, 0.1f + fold), at(0.8f - fold, 0.1f + fold), edge);
		draw->AddRectFilled(at(0.2f, 0.56f), at(0.8f, 0.74f), ToImGuiU32(color));

		std::string extension = entry.Path.extension().string();
		if (!extension.empty())
			extension.erase(0, 1);
		for (char& c : extension)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		if (extension.size() > 5)
			extension.resize(5);
		if (!extension.empty() && size >= 32.0f)
		{
			// Sized to the band, in whichever of the text and page colors reads on it
			ImFont* font = ImGui::GetFont();
			const float fontSize = std::min(size * 0.14f, ImGui::GetFontSize());
			const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, extension.c_str());
			const ImVec2 bandCenter = at(0.5f, 0.65f);
			draw->AddText(font, fontSize, ImVec2(bandCenter.x - textSize.x * 0.5f, bandCenter.y - textSize.y * 0.5f), ToImGuiU32(BandText(color)), extension.c_str());
		}
	}

	void ContentBrowserPanel::DrawEntry(const ContentBrowser::Entry& entry, const OpenSteleFn& openStele)
	{
		const EditorTheme& theme = *m_Theme;
		using Kind = ContentBrowser::Kind;

		// Images show themselves once loaded
		RefCountPtr<ITexture2D> thumbnail;
		if (entry.Type == Kind::Texture)
			thumbnail = m_Assets->GetTexture(m_Model->Import(entry));

		ImGui::PushID(entry.Path.generic_string().c_str());
		ImGui::BeginGroup();
		const bool selected = entry.Path == m_Selected;
		const float width = m_CellSize + ImGui::GetStyle().FramePadding.x * 2.0f;
		const ImVec2 cellMin = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##cell", ImVec2(width, m_CellSize));
		const bool hovered = ImGui::IsItemHovered();
		if (selected || hovered)
		{
			ImGui::GetWindowDrawList()->AddRectFilled(cellMin, ImVec2(cellMin.x + width, cellMin.y + m_CellSize),
			                                          ToImGuiU32(selected ? theme.Highlight : theme.HighlightHover), theme.Rounding);
		}
		DrawIcon(entry, thumbnail, { cellMin.x + (width - m_CellSize) * 0.5f, cellMin.y }, m_CellSize);

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			m_Selected = entry.Path;
		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (entry.Type == Kind::Folder)
				m_Model->Navigate(entry.Path);
			else if (entry.Type == Kind::Stele && openStele)
				openStele(entry.Path);
		}
		if (hovered && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			ImGui::SetTooltip("%s", entry.Path.generic_string().c_str());

		// Meshes and textures can be dragged out (registered as assets on the way)
		if ((entry.Type == Kind::Mesh || entry.Type == Kind::Texture) && ImGui::BeginDragDropSource())
		{
			const AssetDrag drag{ m_Model->Import(entry), entry.Type == Kind::Mesh ? AssetKind::Mesh : AssetKind::Texture };
			ImGui::SetDragDropPayload(kAssetDragType, &drag, sizeof(drag));
			const ImVec2 iconMin = ImGui::GetCursorScreenPos();
			const float iconSize = ImGui::GetFrameHeight() * 1.5f;
			ImGui::Dummy(ImVec2(iconSize, iconSize));
			DrawIcon(entry, thumbnail, { iconMin.x, iconMin.y }, iconSize);
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(entry.Name.c_str());
			ImGui::EndDragDropSource();
		}

		// The name under it, centered, shortened to fit
		const std::string label = Elide(entry.Name, width);
		const float labelWidth = ImGui::CalcTextSize(label.c_str()).x;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (width - labelWidth) * 0.5f));
		if (selected)
			ImGui::TextColored(ToImGui(theme.AccentText), "%s", label.c_str());
		else
			ImGui::TextUnformatted(label.c_str());
		ImGui::EndGroup();
		ImGui::PopID();
	}

}
