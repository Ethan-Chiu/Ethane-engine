#include "ContentBrowserPanel.h"

#include "ContentBrowser/AssetDrag.h"
#include "Ethane/Asset/AssetLibrary.h"
#include "Ethane/Core/Application.h"
#include "Theme/ImGuiTheme.h"
#include "Theme/ImGuiWidgets.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

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

		// A crumb: plain text, a faint wash when hovered
		bool Crumb(const char* label, bool current, const EditorTheme& theme)
		{
			const ImVec2 text = ImGui::CalcTextSize(label);
			const ImVec2 size(text.x + 12.0f, 24.0f);
			const ImVec2 min = ImGui::GetCursorScreenPos();
			const bool clicked = ImGui::InvisibleButton(label, size);
			ImDrawList* draw = ImGui::GetWindowDrawList();
			if (ImGui::IsItemHovered() && !current)
				draw->AddRectFilled(min, ImVec2(min.x + size.x, min.y + size.y), ToImGuiU32(theme.Hover), theme.ControlRounding);
			draw->AddText(ImVec2(min.x + 6.0f, min.y + (size.y - text.y) * 0.5f), ToImGuiU32(current ? theme.Text : theme.TextMuted), label);
			return clicked;
		}

	}

	ContentBrowserPanel::ContentBrowserPanel(ContentBrowser& model, AssetLibrary& assets, const EditorTheme& theme)
		: m_Model(&model), m_Assets(&assets), m_Theme(&theme)
	{
	}

	void ContentBrowserPanel::Draw(bool* open, const OpenSteleFn& openStele)
	{
		BeginPane(kWindowName, open, *m_Theme);
		DrawToolbar();
		DrawGrid(openStele);
		EndPane();
	}

	void ContentBrowserPanel::DrawToolbar()
	{
		const EditorTheme& theme = *m_Theme;
		const float height = 30.0f;
		const float top = ImGui::GetCursorPosY();
		auto center = [&](float h) { ImGui::SetCursorPosY(top + (height - h) * 0.5f); };

		// Back: a round button
		center(26.0f);
		ImGui::BeginDisabled(!m_Model->CanGoUp());
		if (IconButton("##up", Icon::ChevronLeft, 26.0f, theme.TextSecondary, theme, 13.0f, nullptr, 12.0f))
			m_Model->GoUp();
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Up a folder");

		// The roots as segments (assets | res), like the viewport's tool capsule
		if (m_Model->Roots().size() > 1)
		{
			std::vector<std::string> names;
			std::vector<const char*> labels;
			int selected = 0;
			for (const std::filesystem::path& root : m_Model->Roots())
			{
				if (root == m_Model->Root())
					selected = static_cast<int>(names.size());
				names.push_back(root.generic_string());
			}
			for (const std::string& name : names)
				labels.push_back(name.c_str());
			ImGui::SameLine(0.0f, 8.0f);
			center(26.0f);
			const int clicked = SegmentedControl("##roots", labels.data(), static_cast<int>(labels.size()), selected, 26.0f, theme);
			if (clicked >= 0)
				m_Model->Navigate(m_Model->Roots()[clicked]);
		}

		// The folders below the root as crumbs
		const auto crumbs = m_Model->Breadcrumbs();
		for (size_t i = 1; i < crumbs.size(); i++)
		{
			ImGui::SameLine(0.0f, i == 1 ? 10.0f : 0.0f);
			if (i > 1)
			{
				center(ImGui::GetTextLineHeight());
				ImGui::TextColored(ToImGui(EditorTheme::WithAlpha(theme.TextMuted, 0.5f)), "/");
				ImGui::SameLine(0.0f, 0.0f);
			}
			center(24.0f);
			ImGui::PushID(static_cast<int>(i));
			if (Crumb(DisplayName(crumbs[i]).c_str(), i + 1 == crumbs.size(), theme))
				m_Model->Navigate(crumbs[i]);
			ImGui::PopID();
		}

		// Search, at the right: a pill with the magnifier inside
		const float searchWidth = 210.0f;
		const float right = ImGui::GetWindowContentRegionMax().x;
		ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 8.0f, right - searchWidth));
		center(26.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 13.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(28.0f, (26.0f - ImGui::GetFontSize()) * 0.5f));
		ImGui::SetNextItemWidth(searchWidth);
		if (ImGui::InputTextWithHint("##search", "Search", &m_Search))
			m_Model->SetFilter(m_Search);
		ImGui::PopStyleVar(2);
		const ImVec2 field = ImGui::GetItemRectMin();
		Ethane::DrawIcon(ImGui::GetWindowDrawList(), Icon::Search, ImVec2(field.x + 15.0f, field.y + 13.0f), 12.0f, ToImGuiU32(theme.TextMuted));
		ImGui::SetCursorPosY(top + height + 2.0f);
	}

	void ContentBrowserPanel::DrawGrid(const OpenSteleFn& openStele)
	{
		ImGui::BeginChild("##grid");
		// Ctrl + scroll: tile size
		if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f)
			m_CellSize = std::clamp(m_CellSize + ImGui::GetIO().MouseWheel * 8.0f, 40.0f, 160.0f);
		const auto entries = m_Model->Entries();
		if (entries.empty())
			ImGui::TextDisabled(m_Model->Filter().empty() ? "This folder is empty" : "No matches");

		const float gap = 4.0f;
		const float tileWidth = m_CellSize + 24.0f;
		const float available = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(1, static_cast<int>((available + gap) / (tileWidth + gap)));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, gap));
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
		ImGui::PopStyleVar();

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
			case ContentBrowser::Kind::Folder: return m_Theme->Mint;
			case ContentBrowser::Kind::Mesh: return m_Theme->Accent;
			case ContentBrowser::Kind::Texture: return m_Theme->TextSecondary;
			case ContentBrowser::Kind::Stele: return m_Theme->Danger;
			case ContentBrowser::Kind::Other: break;
		}
		return m_Theme->FieldActive;
	}

	void ContentBrowserPanel::DrawIcon(const ContentBrowser::Entry& entry, const RefCountPtr<ITexture2D>& thumbnail, glm::vec2 min, float size) const
	{
		const EditorTheme& theme = *m_Theme;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		auto at = [&](float x, float y) { return ImVec2(min.x + x * size, min.y + y * size); };

		if (thumbnail)
		{
			draw->AddImageRounded(TextureId(thumbnail), at(0.0f, 0.0f), at(1.0f, 1.0f), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, size * 0.125f);
			return;
		}

		if (entry.Type == ContentBrowser::Kind::Folder)
		{
			// The back with its tab, then the front
			const ImU32 back = ToImGuiU32(EditorTheme::WithAlpha(theme.Mint, 0.75f)), front = ToImGuiU32(theme.Mint);
			const float r = size * 0.05f;
			draw->AddRectFilled(at(0.16f, 0.23f), at(0.42f, 0.36f), back, r);
			draw->AddRectFilled(at(0.16f, 0.29f), at(0.84f, 0.78f), back, r);
			draw->AddRectFilled(at(0.16f, 0.36f), at(0.84f, 0.78f), front, r);
			return;
		}

		// A page with a badge carrying the extension
		draw->AddRectFilled(at(0.17f, 0.09f), at(0.83f, 0.91f), ToImGuiU32(theme.FieldHover), size * 0.09f);
		std::string extension = entry.Path.extension().string();
		if (!extension.empty())
			extension.erase(0, 1);
		for (char& c : extension)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		if (extension.size() > 5)
			extension.resize(5);
		if (extension.empty() || size < 32.0f)
			return;
		const EditorTheme::Color badge = KindColor(entry.Type);
		const bool lightBadge = entry.Type != ContentBrowser::Kind::Other;
		ImFont* font = EditorFonts().Mono;
		const float fontSize = std::max(8.0f, size * 0.14f);
		const ImVec2 text = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, extension.c_str());
		const ImVec2 center = at(0.5f, 0.72f);
		const ImVec2 bmin(center.x - text.x * 0.5f - 4.0f, center.y - text.y * 0.5f - 1.0f), bmax(center.x + text.x * 0.5f + 4.0f, center.y + text.y * 0.5f + 1.0f);
		draw->AddRectFilled(bmin, bmax, ToImGuiU32(badge), 3.0f);
		draw->AddText(font, fontSize, ImVec2(center.x - text.x * 0.5f, center.y - text.y * 0.5f),
		              ToImGuiU32(lightBadge ? theme.Pane : theme.TextSecondary), extension.c_str());
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
		const bool selected = entry.Path == m_Selected;
		const float width = m_CellSize + 24.0f;
		const float labelHeight = ImGui::GetFontSize() + 4.0f;
		const ImVec2 size(width, 8.0f + m_CellSize + 6.0f + labelHeight + 4.0f);
		const ImVec2 min = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##tile", size);
		const bool hovered = ImGui::IsItemHovered();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		if (selected || hovered)
			draw->AddRectFilled(min, ImVec2(min.x + size.x, min.y + size.y), ToImGuiU32(selected ? theme.Selected : theme.Hover), theme.CardRounding);
		DrawIcon(entry, thumbnail, { min.x + (width - m_CellSize) * 0.5f, min.y + 8.0f }, m_CellSize);

		// The name under it, centered, shortened to fit
		ImGui::PushFont(nullptr, theme.FontSize - 2.0f);
		const std::string label = Elide(entry.Name, width - 8.0f);
		const ImVec2 text = ImGui::CalcTextSize(label.c_str());
		draw->AddText(ImVec2(min.x + (width - text.x) * 0.5f, min.y + 8.0f + m_CellSize + 6.0f), ToImGuiU32(selected ? theme.Text : theme.TextSecondary), label.c_str());
		ImGui::PopFont();

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
			const float iconSize = 32.0f;
			ImGui::Dummy(ImVec2(iconSize, iconSize));
			DrawIcon(entry, thumbnail, { iconMin.x, iconMin.y }, iconSize);
			ImGui::SameLine();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (iconSize - ImGui::GetFontSize()) * 0.5f);
			ImGui::TextUnformatted(entry.Name.c_str());
			ImGui::EndDragDropSource();
		}
		ImGui::PopID();
	}

}
