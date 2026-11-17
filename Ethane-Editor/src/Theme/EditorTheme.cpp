#include "EditorTheme.h"

namespace Ethane {

	EditorTheme EditorTheme::Light()
	{
		EditorTheme theme;
		theme.Name           = "Light";
		theme.Background     = Hex(0xFFF3E3); // cream
		theme.Panel          = Hex(0xF7E8D3);
		theme.Surface        = Hex(0xFFFBF5);
		theme.SurfaceHover   = Hex(0xE6F1EF);
		theme.SurfaceActive  = Hex(0xD5E8E6);
		theme.Border         = Hex(0xEBDAC3);

		theme.Text           = Hex(0x2B2D3A);
		theme.TextMuted      = Hex(0x968B7C);

		theme.Highlight      = Hex(0xD5E8E6); // mint
		theme.HighlightHover = Hex(0xE4F0EE);
		theme.Accent         = Hex(0x9FA0FF); // periwinkle
		theme.AccentText     = Hex(0x6668D9);
		theme.Secondary      = Hex(0x6FB3AA); // mint, deepened to read on cream
		theme.Danger         = Hex(0xFF7171); // coral
		theme.Selection      = Hex(0x9FA0FF);
		return theme;
	}

	EditorTheme EditorTheme::Dark()
	{
		EditorTheme theme;
		theme.Name           = "Dark";
		theme.Background     = Hex(0x1D1D20);
		theme.Panel          = Hex(0x252528);
		theme.Surface        = Hex(0x2E2E32);
		theme.SurfaceHover   = Hex(0x38383D);
		theme.SurfaceActive  = Hex(0x424248);
		theme.Border         = Hex(0x323236);

		theme.Text           = Hex(0xFFF3E3); // cream
		theme.TextMuted      = Hex(0x8D8A85);

		theme.Highlight      = Hex(0x33484A); // mint, darkened
		theme.HighlightHover = Hex(0x2B393B);
		theme.Accent         = Hex(0x9FA0FF); // periwinkle
		theme.AccentText     = Hex(0xB3B4FF);
		theme.Secondary      = Hex(0xD5E8E6); // mint
		theme.Danger         = Hex(0xFF7171); // coral
		theme.Selection      = Hex(0x9FA0FF);
		return theme;
	}

}
