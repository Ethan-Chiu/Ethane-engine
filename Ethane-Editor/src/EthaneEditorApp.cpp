#include "Ethane.h"
//-----EntryPoint-----
#include "Ethane/Core/EntryPoint.h"

#include "EditorLayer.h"
#include "TriangleLayer.h"
#include "TexturedQuadLayer.h"
#include "CubeLayer.h"

namespace Ethane {

	class EthaneEditor : public Application
	{
	public:
		EthaneEditor(ApplicationCommandLineArgs args)
			:Application("Ethane Editor", args)
		{
			// The editor UI; the 3D scene (DemoScene) renders into its Viewport panel
			PushLayer(new EditorLayer());
			// Window-pass demos: they draw directly into the window, underneath the editor's dockspace (which
			// covers it). To see one, push it instead of EditorLayer.
			// PushLayer(new TriangleLayer());
			// PushLayer(new TexturedQuadLayer());
			// PushLayer(new CubeLayer());
		}

		~EthaneEditor()
		{

		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		return new EthaneEditor(args);
	}

}
