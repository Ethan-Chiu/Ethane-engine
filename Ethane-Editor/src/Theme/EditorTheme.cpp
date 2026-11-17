#include "EditorTheme.h"

namespace Ethane {

	EditorTheme EditorTheme::Default()
	{
		EditorTheme theme;
		theme.Background    = Hex(0x18191F);
		theme.Panel         = Hex(0x1F2029);
		theme.Surface       = Hex(0x292B37);
		theme.SurfaceHover  = Hex(0x333545);
		theme.SurfaceActive = Hex(0x3D4054);
		theme.Border        = Hex(0x323445);

		theme.Text          = Hex(0xFFF3E3); // cream
		theme.TextMuted     = Hex(0x9EA3B5);

		theme.Accent        = Hex(0x9FA0FF); // periwinkle
		theme.AccentHover   = Hex(0xB7B8FF);
		theme.AccentActive  = Hex(0x8586F0);
		theme.Secondary     = Hex(0xD5E8E6); // mint
		theme.Danger        = Hex(0xFF7171); // coral
		theme.Selection     = theme.Accent;
		return theme;
	}

}
