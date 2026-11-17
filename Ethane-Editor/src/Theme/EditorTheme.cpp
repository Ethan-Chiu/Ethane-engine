#include "EditorTheme.h"

namespace Ethane {

	EditorTheme EditorTheme::Dark()
	{
		const Color cream = Hex(0xFFF3E3), mint = Hex(0xD5E8E6), periwinkle = Hex(0x9FA0FF), coral = Hex(0xFF7171);

		EditorTheme theme;
		theme.Window        = Hex(0x0E0E10);
		theme.Pane          = Hex(0x1A1A1E);
		theme.Card          = Hex(0x202025);
		theme.Field         = Hex(0x26262C);
		theme.FieldHover    = Hex(0x2D2D34);
		theme.FieldActive   = Hex(0x34343C);
		theme.Line          = Hex(0x26262C);

		theme.Text          = cream;
		theme.TextSecondary = Hex(0xC9C3B8);
		theme.TextMuted     = Hex(0x8C8A85);
		theme.TextFaint     = Hex(0x6E6C68);

		theme.Hover         = WithAlpha(cream, 0.05f);
		theme.Selected      = WithAlpha(mint, 0.11f);

		theme.Accent        = periwinkle;
		theme.Mint          = mint;
		theme.Danger        = coral;
		theme.AxisX         = coral;
		theme.AxisY         = mint;
		theme.AxisZ         = periwinkle;

		theme.Glass         = Hex(0x24242A, 0.88f);
		theme.GlassEdge     = WithAlpha(cream, 0.08f);
		theme.GlassSelected = WithAlpha(cream, 0.12f);
		return theme;
	}

}
